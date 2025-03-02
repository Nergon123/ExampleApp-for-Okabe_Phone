# EXAMPLE APPLICATION FOR [OKABE PHONE](https://github.com/Nergon123/Okabe_Phone)

## What is it?

So at some point, I decided that I could implement executing third-party binaries from Okabe Phone.

This repository is a simple "Hello world" program with some useful functions in the `DRIVER` class.

## How does it work?

Okabe_Phone main firmware doesn't take much space (around 550 KB when I'm writing this). This phone has 4Mb flash. I divided it into partitions (1 Mb for the main app and the rest for the 3rd party one ~3 MB).
When you execute the application device loads it into the second app partition and boots it from there (The device reboots when this happens). So any project that uses ESP32, a screen library that supports ILI9341 (maybe it will be another one after I finish the final board) and buttons can be easily ported.

## What can I write with it?
Anything really (except for malicious, well you can technically write them but I do not approve of those). You are limited only by hardware and Arduino libraries.

