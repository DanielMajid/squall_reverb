# Squall Reverb

`Squall` is a Clouds-inspired stereo reverb unit for the Korg NTS-1 mkII.

Ported by Daniel Majid Mirzakhani.

Original Clouds DSP copyright Émilie Gillet.

## Highlights

- Uses DSP from the original Clouds synth module for an accurate emulation.
- Playable `DPTH` range from short ambience to near-infinite sustain tails.
- Knob B `DPTH` also controls wet amount to closely emulate the 'one knob' functionality of the original module. Standard wet/dry functionality is thereby disabled.
- `FREEZE` parameter emulates the freeze functionality from the original module.

## Controls

- Knob A `TONE`: Gentle low-pass control on reverb tail.
- Knob B `DPTH`: Reverb amount/decay character.
- `DELAY` shift + Knob B `MIX`: Wet/dry mix is currently disabled.
- `FREEZE`: Freezes the reverb buffer.

`FREEZE` behavior:

- `< 50%`: FREEZE off (normal operation).
- `>= 50%`: FREEZE on .

When FREEZE is engaged:

- New input stops exciting the reverb network.
- Existing tail content is held/sustained and remains playable with controls.
- Reverb amount is forced fully wet and damping shifts to keep the wash stable.

`DPTH` behavior:

- `0-50%`: Room/hall style reverb.
- `50-75%`: Longer cathedral-like tail with subtle modulation character.
- `75-99%`: Very long dreamy tail with more obvious modulation in the decay.
- `100%`: 'infinite' reverb tail.

## To build this project

- Clone this repo 
	In desired directory:

	Download this repo

	git clone --recurse-submodules https://github.com/DanielMajid/squall_reverb.git

- Download the ARM GCC toolchain

	cd logue-sdk/tools/gcc/
	./get_gcc_osx.sh
	Run Make command to build binary

- Compile project
	Run "make install"
	Open Korg Kontrol Editor

- Load Project
	Drag .nts1mkiiunit file into the appropriate module category
	Click sync

## License

Released under the GPL3.0 license. See `LICENSE`.
