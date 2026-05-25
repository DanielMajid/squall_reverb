# Squall (NTS-1 mkII)

`Squall` is a Clouds-inspired stereo reverb unit for the Korg NTS-1 mkII (`revfx` module).

Ported by Daniel Majid Mirzakhani.

Original Clouds DSP copyright Emilie Gillet.

## Highlights

- Clouds-style long-tail reverb behavior tuned for NTS-1 mkII revfx workflow.
- Playable `DPTH` range from short ambience to near-infinite sustain tails.
- Dedicated `MIX` dry/wet control on shift-knob path.

## Controls

- `Knob A (TONE)`: Gentle low-pass control on reverb tails.
- `Knob B (DPTH)`: Reverb amount/decay character.
- `DELAY shift + Knob B (MIX)`: Dry/wet balance.

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

Released under the MIT license. See `LICENSE.md`.
