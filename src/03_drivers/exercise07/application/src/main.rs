use std::fs::OpenOptions;
use std::io::{Read, self, Write};
use std::os::unix::fs::OpenOptionsExt;
use std::os::unix::io::AsRawFd;
use mio::{Events, Interest, Poll, Token};
use mio::unix::SourceFd;

const SWITCH_TOKEN: Token = Token(0);

fn main() -> io::Result<()> {
    let mut total_interrupts = 0;
    // Reset the interrupt counter by writing "0" to the sysfs attribute.
    let mut sysfs_file = OpenOptions::new()
        .write(true)
        .open("/sys/class/misc/switch_driver/nb_of_interrupts")?;
    sysfs_file.write_all(b"0")?;
    println!("Reset interrupt counter.");

    // Open the character device file provided by the driver.
    let mut device = OpenOptions::new()
        .read(true) // We want to read from the device.
        .custom_flags(libc::O_NONBLOCK) // Use custom flags to open in non-blocking mode.
        .open("/dev/switch_driver")?;

    println!("Opened driver device: /dev/switch_driver");

    let mut poll = Poll::new()?;
    // Create an `Events` collection to store events from the poller.
    let mut events = Events::with_capacity(128);

    // Register the device's file descriptor with the `Poll` instance.
    // We are interested in `READABLE` events, which occur when there is data to be read.
    poll.registry().register(
        &mut SourceFd(&device.as_raw_fd()),
        SWITCH_TOKEN,
        Interest::READABLE,
    )?;

    println!("Waiting for data from the driver...");

    loop {
        // `poll.poll()` blocks until at least one event occurs.
        // `None` for the timeout means it will wait indefinitely.
        poll.poll(&mut events, None)?;

        for event in events.iter() {
            // Check if the event's token matches our device's token.
            match event.token() {
                SWITCH_TOKEN => {
                    // Check if the event indicates that the source is readable.
                    if event.is_readable() {
                        let mut buffer = [0u8; 1024];
                        match device.read(&mut buffer) {
                            Ok(bytes_read) => {
                                if bytes_read > 0 {
                                    // Convert the read bytes to a string and print it.
                                    let n_interrupts_str = String::from_utf8_lossy(&buffer[..bytes_read]);
                                    total_interrupts += n_interrupts_str.trim().parse::<u32>().unwrap_or(total_interrupts);
                                    println!("Received data from driver: {} (Total interrupts: {})", n_interrupts_str.trim(), total_interrupts);
                                }
                            }
                            // If the read would block, it means no data is available right now.
                            Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
                                continue;
                            }
                            // Handle other potential read errors.
                            Err(e) => {
                                return Err(e);
                            }
                        }
                    }
                }
                _ => {}
            }
        }
    }
}