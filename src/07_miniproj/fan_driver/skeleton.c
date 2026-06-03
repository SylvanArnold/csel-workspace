// fan_controller.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/timer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <linux/device.h>


// Fan controller device state
struct fan_ctrl {
	struct thermal_zone_device *tz; //Thermal zone device to get cpu temp
	struct timer_list           timer; // Timer for toggling the fan GPIO
	struct device              *dev; // device for logging and sysfs
	
	int gpio_pin; // GPIO pin controlling the fan
	int gpio_state;    
	spinlock_t lock; // Spinlock to protect sysfs attributes
	
	// Sysfs attributes
	bool       manual_mode;  
	u32        frequency;     
};

// Lookup table for temperature to frequency mapping
static u32 temp_to_freq(s32 temp_mc) {
	if (temp_mc < 35000)
		return 2;
	if (temp_mc > 40000)
		return 5;
	if (temp_mc < 45000)
		return 10;
	
	return 20;
}

static void fan_toggle_timer(struct timer_list *t) {
	struct fan_ctrl *fc = from_timer(fc, t, timer);
	u32  freq;
	bool manual;
	int  ret;
	spin_lock(&fc->lock);
	manual = fc->manual_mode;
	freq   = fc->frequency;
	spin_unlock(&fc->lock);

	// In automatic mode, read temperature and update frequency
	if (!manual) {
		s32 temp_mc = 0;

		ret = thermal_zone_get_temp(fc->tz, &temp_mc);
		if (ret) {
			dev_warn_ratelimited(fc->dev,
				"thermal read failed (%d), keeping last frequency\n",
				ret);
		} else {
			freq = temp_to_freq(temp_mc);

			spin_lock(&fc->lock);
			fc->frequency = freq;
			spin_unlock(&fc->lock);
		}
	}

	// If frequency is zero, turn off the GPIO and return without rescheduling
	if (freq == 0) {
		fc->gpio_state = 0;
		gpio_set_value(fc->gpio_pin, 0);
		return; 
	}

	// Toggle GPIO state and set the pin
	fc->gpio_state ^= 1;
	gpio_set_value(fc->gpio_pin, fc->gpio_state);

	// Schedule next toggle
	mod_timer(&fc->timer,
		  jiffies + msecs_to_jiffies(1000 / (2 * freq)));
}

/* ------------------------------------------------------------------ */
/*  sysfs attributes                                                   */
/* ------------------------------------------------------------------ */

static ssize_t manual_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf) {
	struct fan_ctrl *fc = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n", fc->manual_mode);
}

static ssize_t manual_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count) {
	struct fan_ctrl *fc = dev_get_drvdata(dev);
	bool val;

	if (kstrtobool(buf, &val))
		return -EINVAL;

	spin_lock(&fc->lock);
	fc->manual_mode = val;
	spin_unlock(&fc->lock);

	return count;
}

static ssize_t frequency_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct fan_ctrl *fc = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n", fc->frequency);
}

static ssize_t frequency_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count) {
	struct fan_ctrl *fc = dev_get_drvdata(dev);
	u32 val;

	if (kstrtou32(buf, 10, &val))
		return -EINVAL;

	spin_lock(&fc->lock);
	if (!fc->manual_mode) {
		spin_unlock(&fc->lock);
		dev_warn(dev, "switch to manual mode first\n");
		return -EPERM;
	}
	fc->frequency = val;
	spin_unlock(&fc->lock);

	// If frequency is zero, turn off the GPIO and return without rescheduling the timer
	// Otherwise, calculate the new timer period and reschedule the timer
	if (fc->frequency == 0) {
		fc->gpio_state = 0;
		gpio_set_value(fc->gpio_pin, 0);
	} else{
		unsigned long period_ms = 1000 / (2 * fc->frequency);
		mod_timer(&fc->timer, jiffies + msecs_to_jiffies(period_ms));
	}

	return count;
}

// Read-only attribute to show current CPU temperature in millidegrees Celsius
static ssize_t cpu_temp_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
	struct fan_ctrl *fc = dev_get_drvdata(dev);
	s32 temp_mc = 0;
	int ret;

	ret = thermal_zone_get_temp(fc->tz, &temp_mc);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read cpu temperature\n");

	return sysfs_emit(buf, "%d\n", temp_mc);
}

static DEVICE_ATTR_RO(cpu_temp);
static DEVICE_ATTR_RW(manual_mode);
static DEVICE_ATTR_RW(frequency);

static struct attribute *fan_attrs[] = {
	&dev_attr_manual_mode.attr,
	&dev_attr_frequency.attr,
	&dev_attr_cpu_temp.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fan);


// Probe is called at module load time
static int fan_ctrl_probe(struct platform_device *pdev)
{
	struct device      *dev = &pdev->dev;
	struct device_node *np  = dev->of_node;
	struct fan_ctrl    *fc;
	int ret;

	fc = devm_kzalloc(dev, sizeof(*fc), GFP_KERNEL);
	if (!fc)
		return -ENOMEM;

	fc->dev = dev;
	spin_lock_init(&fc->lock);

	fc->gpio_pin = of_get_named_gpio(np, "fan-gpios", 0);
	if (!gpio_is_valid(fc->gpio_pin))
		return dev_err_probe(dev, -EINVAL, "invalid fan GPIO\n");

	ret = gpio_request(fc->gpio_pin, "fan-ctrl");
	if (ret)
		return dev_err_probe(dev, ret, "failed to request fan GPIO\n");

	ret = gpio_direction_output(fc->gpio_pin, 0);
	if (ret) {
		gpio_free(fc->gpio_pin);
		return dev_err_probe(dev, ret,
				     "failed to set GPIO as output\n");
	}

	fc->tz = thermal_zone_get_zone_by_name("cpu-thermal");
	if (IS_ERR(fc->tz)) {
		gpio_free(fc->gpio_pin);
		return dev_err_probe(dev, PTR_ERR(fc->tz),
				     "failed to get thermal zone\n");
	}

	fc->manual_mode = false;
	fc->frequency   = 0;
	fc->gpio_state  = 0;

	platform_set_drvdata(pdev, fc);

	ret = devm_device_add_groups(dev, fan_groups);
	if (ret) {
		gpio_free(fc->gpio_pin);
		return dev_err_probe(dev, ret, "failed to create sysfs groups\n");
	}

	timer_setup(&fc->timer, fan_toggle_timer, 0);

	fan_toggle_timer(&fc->timer);

	dev_info(dev, "fan controller ready (GPIO %d)\n", fc->gpio_pin);
	return 0;
}

static int fan_ctrl_remove(struct platform_device *pdev)
{
	struct fan_ctrl *fc = platform_get_drvdata(pdev);

	del_timer_sync(&fc->timer);
	gpio_set_value(fc->gpio_pin, 0);
	gpio_free(fc->gpio_pin);

	return 0;
}

// Device tree match table
static const struct of_device_id fan_ctrl_of_match[] = {
	{ .compatible = "vendor,fan-controller" },
	{ }
};
MODULE_DEVICE_TABLE(of, fan_ctrl_of_match);

static struct platform_driver fan_ctrl_driver = {
	.probe  = fan_ctrl_probe,
	.remove = fan_ctrl_remove,
	.driver = {
		.name           = "fan-controller",
		.of_match_table = fan_ctrl_of_match,
	},
};

module_platform_driver(fan_ctrl_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sylvan Arnold");
MODULE_DESCRIPTION("GPIO fan controller with thermal feedback");