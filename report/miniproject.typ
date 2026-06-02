#let custom-title-page(
  title: "Linux Embedded Systems",
  subtitle: "Mini Project Report",
  student: "Sylvan Arnold",
  program: "Computer Science",
  class_name: "MA_CSEL1",
  academic_year: "2026",
  repository: "https://github.com/SylvanArnold/csel-workspace",
) = [
  #align(center, [
    #image("ressources/hesso-logo.svg", width: 42mm)
    #v(12mm)
    #text(size: 33pt, weight: "bold", fill: rgb("#0f172a"))[#title]
    #v(4mm)
    #text(size: 16pt, fill: rgb("#334155"))[#subtitle]
    #v(9mm)
    #rect(width: 78%, height: 1pt, fill: rgb("#94a3b8"))
  ])

  #v(10mm)

  #block(
    fill: rgb("#f8fafc"),
    stroke: 0.7pt + rgb("#cbd5e1"),
    radius: 8pt,
    inset: 14pt,
  )[
    #table(
      columns: (35%, 65%),
      stroke: none,
      inset: (x: 6pt, y: 4pt),
      [Student], [#student],
      [Program], [#program],
      [Academic year], [#academic_year],
      [Class], [#class_name],
      [Repository], [https://github.com/SylvanArnold/csel-workspace],
    )
  ]

  #v(23mm)

  #align(center, text(size: 10pt, fill: rgb("#64748b"))[
    HES-SO // Spring 2026
  ])

  #pagebreak()
]

#custom-title-page()

= Introduction

You can find the source code of this mini project under the `src/07_miniproj` folder of the repository. 


= Kernel Module

To control the fan, I created a module_platform_driver installable with `modprobe`. When loaded, the module will request the `fan-ctrl` pin from the device tree.

I added the following node to the device tree:

```
    fan-controller {
        compatible = "vendor,fan-controller";
        fan-gpios = <&pio 0 10 GPIO_ACTIVE_HIGH>;
        status = "okay";
    };
```

This node connects the module to the Status led pin of the board since we don't have a real fan to control.

I had to rebuild my image on buildroot to apply the changes to the device tree.

I created two sysfs entries to control the fan: `manual_mode`, which allows to switch between manual and automatic mode, and `frequency`, which allows to set the frequency of the fan in manual mode and read the current frequency in automatic mode.

In automatic mode, the frequency is set according to the CPU temperature, which is read from the `cpu-thermal` zone. I implemented a simple lookup table as requested in the instructions.

I built and installed the module on the board and everything works as expected.

= Daemon

The daemon uses the screen and power leds. I had to add the following nodes to the device tree to use them:

```
/ {
    /delete-node/ leds;
};

&i2c0 {
        status = "okay";
};
```

= User Space Application