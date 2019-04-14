--> TLDR run `./shipping_party` if you're an orga. <--

International Shipping is a 4k gfx demo for 64-bit linux, specifically targeting Ubuntu 18.10 with NVidia graphics cards and proprietary drivers.

Packages needed:
(all of these are by default installed on Ubuntu after installing NVidia drivers.)

libglib2.0-0
libgtk-3-0
libfftw3-single3
nvidia-###, where ### is the latest driver number

Execution instructions:

NOTE TO ORGAS: please run `./shipping_party` and not `./shipping`

To execute the demo, run the following command in the directory with the demo:

~$ ./shipping

By default this will render with 1 sample per pixel, on my machine (i7-3720QM/Quadro K1000M) this takes about 1 second. You can increase the number of samples with an environment variable:

~$ SAMPLES=10 ./shipping

This takes 3 seconds on my machine. The reason it is not 10 seconds is because there is a large precalc overhead at the start of the demo. Subsequent samples do not have this overhead.

The party version (`./shipping_party`) has the default number of samples set to 1000. This number can also be modified using the `SAMPLES` environment variable.

See the .nfo file for render timings on a K1000M and a 980 Ti.
