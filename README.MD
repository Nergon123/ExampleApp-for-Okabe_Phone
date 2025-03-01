# EXAMPLE APPLICATION FOR [OKABE PHONE](https://github.com/Nergon123/Okabe_Phone)

## What it is?

So at some point I decided that I could implement executing third party binaries from Okabe Phone.

This repository is simple `"Hello world"` program with some useful functions in `DRIVER` class.

## How does it works?

Okabe_Phone main firmware doesn't take much space (around 550 Kb at moment when I writing this).This phone have 4Mb flash. I divided it to partitions (1 Mb for the main app and rest for the 3rd party one ~3Mb).
When you executing application device is loading them into second app partition and boot it from there (Device reboots when this happens). So any project that use ESP32, screen library that supports ILI9341 (maybe it will be other one after I finish final board) and buttons can be easily ported.

## What can I write with it?
Anything really (except for malicious, well you can technically write them but I do not approve those).You are limited only by hardware and arduino libraries.


