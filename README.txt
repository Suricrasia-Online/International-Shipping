International Shipping is a 4k gfx demo for 64-bit linux, specifically targeting Ubuntu 18.10.

Packages needed:
(all of these are by default installed on Ubuntu after installing NVidia drivers.)

libglib2.0-0
libgtk-3-0
libfftw3-single3
some kind of graphics driver (mesa or nvidia are fine)

Execution instructions:

To execute the demo, run the following command in the directory with the demo:

~$ ./shipping

By default this will render with 1000 samples per pixel, on my machine (i7-3720QM/Quadro K1000M) this takes about 165 seconds. You can choose the number of samples you want with an environment variable:

~$ SAMPLES=10 ./shipping

This takes 3 seconds on my machine. See the .nfo file for render timings on a K1000M and a 980 Ti. Please note, it will only render if your screen is at least 1920 pixels wide.
