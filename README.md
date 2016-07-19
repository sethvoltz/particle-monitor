Particle Monitor-Monitor
========================

Show which monitor is currently active while fullscreen. This works in conjunction with software on your desktop to send the set commands.

Usage
-----

Send commands to the device over USB Serial. On macOS you can find the active device by running:

```(bash)
ioreg -c IOSerialBSDClient -r -t \
| awk 'f;/Photon/{f=1};/IOCalloutDevice/{exit}' \
| sed -n 's/.*"\(\/dev\/.*\)".*/\1/p'
```

Serial commands are:

`<display>` is the 32-bit unsigned ID of the display. `<indicator>` is the zero-based index of which indicator should light up when a display is set active.

- `list` -- List all displays in memory, this runs during startup
- `add <display> <indicator>` -- Add or update a display from memory
- `remove <display>` -- Remove a display from memory
- `set <display>` -- Set a display as the current active, will unset all others

Development
-----------

Build and test the Monitor-Monitor by running the following command. This will compile the code on the Particle cloud, load it onto the device and wait a moment before opening the serial terminal.

```
$ particle compile photon ./ --saveTo firmware.bin && \
particle flash <device_name> firmware.bin && \
sleep 7 && particle serial monitor
```

In a second terminal you can send commands to the device with echo via USB serial. The `<device_number>` is the one displayed when the particle serial monitor loads. See _Usage_ above for commands.

```
$ echo "set three" > /dev/cu.usbmodem<device_number>
```
