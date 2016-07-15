Particle Monitor-Monitor
========================

Show which monitor is currently active while fullscreen. This works in conjunction with software on your desktop to send the set commands.

Usage
-----

Serial send to the device a `set` command followed by the monitor ID.

<TBD> How to use the Particle cloud to set config values for monitors and their position on the LED chain.

Development
-----------

Build and test the Monitor-Monitor by running the following command. This will compile the code on the Particle cloud, load it onto the device and wait a moment before opening the serial terminal.

```
$ particle compile photon ./ --saveTo firmware.bin && \
particle flash <device_name> firmware.bin && \
sleep 7 && particle serial monitor
```

In a second terminal you can send commands to the device with echo via USB serial. The `<device_number>` is the one displayed when the particle serial monitor loads.

```
$ echo "set three" > /dev/cu.usbmodem<device_number>
```
