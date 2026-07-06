# Squall Reverb

`Squall` is a port of Mutable Instruments Clouds reverb for the Korg NTS-1 mkII.

Ported by Daniel Majid Mirzakhani.

Original Clouds DSP copyright Émilie Gillet.

## Highlights

- Uses DSP from the original Clouds synth module for an accurate emulation of the onboard reverb.
- Playable `DPTH` range from short ambience to long dreamy washes.
- Knob B `DPTH` also controls wet amount to closely emulate the 'one knob' functionality of the original module.
- `FREEZE` holds the reverb tail and limits fresh input.
- `SCAN` from `50%` to `100%` colors the frozen buffer. From `50%` to `0%` increases buffer leak to allow in new audio.

## Controls

- Knob A `TONE`: Gentle low-pass control on reverb tail.
- Knob B `DPTH`: Reverb amount/decay character.
- `MIX` Hold `REVERB` button + Knob B: Wet/dry mix is currently disabled.
- `FREEZE` Hold `REVERB` button + Knob B: Freezes the reverb buffer.
    - Knob B `< 50%`: FREEZE off (normal operation).
    - Knob B `>= 50%`: FREEZE on.
    - Existing tail content is held.
- `SCAN` Freeze behavior shaper.
    - `50-100%`: Full clamp closed (no input bleed, no leak-out) with increasing frozen color sweep.
    - `0-50%`: Allows fresh-input bleed (up to 5%) while leak-out is fixed at only 0.5%.
    - `0%`: Maximum input bleed and darker/less diffuse freeze color.


`DPTH` behavior:

- `0-50%`: Room/hall style reverb.
- `50-75%`: Longer cathedral-like tail with subtle modulation character.
- `75-100%`: Very long dreamy tail with more obvious modulation in the decay.

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
    - Run `make install`.
    - Open Korg Kontrol Editor.

- Load Project
    - Drag `.nts1mkiiunit` file into the appropriate module category.
    - Click sync.

## License

Released under the GPL3.0 license. See `LICENSE`.
