# Squall (NTS-1 mkII)

`Squall` is a port of the reverb engine from the Clouds eurorack module by Mutable Instruments.  

Ported as a stereo reverb unit for the Korg NTS-1 mkII (`revfx` module) by Daniel Majid Mirzakhani.

Original Clouds DSP copyright Emilie Gillet.

## Highlights

- Clouds-style long-tail reverb behavior tuned for NTS-1 mkII revfx workflow.
- Playable `DPTH` range  from short ambience to 'infinite' sustain tails. `Knob B (DPTH)` controls wet amount to closely emulate the 'one knob' functionality of the original Clouds module.

## Controls

- `Knob A (TONE)`: Gentle low-pass control on reverb tails.
- `Knob B (DPTH)`: Reverb amount/decay character.
- `DELAY shift + Knob B (MIX)`: Dry/wet balance is currently disabled.

`DPTH` behavior:

- `0-50%`: Room/hall style reverb.
- `50-75%`: Longer cathedral-like tails with subtle modulation character.
- `75-99%`: Very long dreamy tails with more obvious modulation in the decay.
- `100%`: Near-infinite tail behavior.

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

Released under the MIT license. See `LICENSE`.
