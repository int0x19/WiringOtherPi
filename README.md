# WiringOtherPi README

An initiative to creat a unified WiringPI port to support all H2/H3 boards

## background

It's a fork of a fork of a fork

WiringPI was ported to BananaPI.  There was an OrangePi fork of the BananaPi fork.  There's enough strong code in the Sunxi community that we're in a position to detect what H3/H2 Boards we are using, so this is an attempt to unify the WiringPI library for AllWinner H2/H3 SBCs.

## Get Involved

All discussion is happening on the Armbian Forums on [this topic](https://forum.armbian.com/index.php/topic/2956-559-gpio-support-for-h2h3-boards-with-a-unified-wiringpi-library-in-a-neat-little-package/)

## General Goals

* WiringPI GPIO support for most AllWinner H2/H3 boards using legacy kernel
* WiringPI GPIO support for most AllWinner H2/H3 boards using mainline kernel
* Compatiblity with code using WiringPI dependencies
* Make my Orange Pi Zero blink my christmas lights
* Produce a distro-friendly package to be included with Armbian releases
