# IOS-Z80-MBC2
This is a copy of the Arduino source for the Z80-MBC2 computer (https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer). The reason that I have created this copy is to allow me to work on my own changes using GIT to provide change control. While GitHub (https://github.com/SuperFabius/Z80-MBC2) holds some of the source and SD Card files these don't appear to be up to date with the lastest version in hackaday.io and the author doesn't appear to be using GIT to control their revisions.

I really like the ability to boot multiple OSes from one SD card.

Improvements I intend to make:
* Add new op code to allow reading of millis
* Increase the SD card buffer size to improve read performance.
* Provide an interface to allow Z80 code to make IO requests to the I2C bus.
* Change to use standard SD library to access the SD card.
* Remove hardcoded information about OSes in disk sets and move the information into the .DAT file. This should mean being able to add new disk sets without having to change the version of IOS.

