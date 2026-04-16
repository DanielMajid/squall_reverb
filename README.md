# Squall

A port of Mutable Instruments Clouds reverb effect for the Korg NTS-1 mkii

Knob A: time </br>
Knob B: depth </br>
Shift Menu : wet/dry control </p>

depth from 0 - 50% produces pleasant room/hall style reverb </br>
depth from 50 - 75% gives lush cathedral style reverb with long tail and tails begin to feature slight modulation </br>
depth from 75 - 99% gives lengthy dreamy reverb with more recognizable tail modulation </br>
depth at 100% creates near "infinite" tail </p>

To build this project: </p>

In desired directory: </br>

Download this repo</br>
- daf
- git submodule update --init</p>

Update logue sdk dependencies
- cd logue-sdk
- git submodule update --init
- tools/gcc/get_gcc_osx.sh

Run Make command to build binary
- run "make install"

Open Korg Kontrol Editor
Drag .nts1mkiiunit file into the appropriate module category
Click sync
Enjoy the reverberation!

port by Daniel Majid Mirzakhani
original Clouds DSP by Emilie Gillet

released under MIT license see license.txt for more information
