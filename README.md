# Ski waxing iron.

Goal
-------
Skiing fast )

Upload firmware with avr-dude (use optiboot arduino bootloader)
avrdude -C/usr/share/arduino/hardware/tools/avrdude.conf -v -v -v -v -patmega328p -carduino -P/dev/ttyUSB2 -b115200 -D -Uflash:w:/tmp/build4688977123459503696.tmp/Blink.cpp.hex:i
