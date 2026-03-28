use std::fs::OpenOptions;
use std::io::{Read, Write, self};

fn main() -> io::Result<()> {
    let num_devices = 3;
    let mut devices = Vec::new();

    for i in 0..num_devices {
        let device = OpenOptions::new()
            .read(true)
            .write(true)
            .open(format!("/dev/skeleton{}", i))?;
        devices.push(device);
    }

    println!("Opened all devices");


    for i in 0..num_devices {
        let message = format!("Hello, device {}", i);
        devices[i].write_all(message.as_bytes())?;
    }

    let mut buffer = [0u8; 1024];

    for i in 0..num_devices {
        let bytes_read = devices[i].read(&mut buffer)?;
        let response = String::from_utf8_lossy(&buffer[..bytes_read]);
        println!("Response from device {}: {}", i, response);
    }

    Ok(())
}