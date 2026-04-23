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
- git clone --recurse-submodules https://github.com/DanielMajid/squall_reverb.git</br>

Download the ARM GCC toolchain</br>
- cd logue-sdk/tools/gcc/
- ./get_gcc_osx.sh</br>

Run Make command to build binary</p>
- run "make install"</br>

Load the unit
- Open Korg Kontrol Editor
- Drag .nts1mkiiunit file into the appropriate module category
- Click sync</p>
Enjoy the reverberation!</br>

Port by Daniel Majid Mirzakhani</p>
Original Clouds DSP by Emilie Gillet</br>

released under MIT license see license.txt for more information
