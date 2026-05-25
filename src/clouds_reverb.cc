/*
 * File: clouds_reverb.cc
 *
 * NTS-1 mkII reverb bridge for the Clouds reverb core.
 *
 * This file owns runtime hook wiring, parameter forwarding, and delay-memory
 * handoff. DSP behavior is implemented in src/reverb.h.
 *
 * Clouds DSP Copyright (c) 2014 Emilie Gillet released under GPL3.0 License. See LICENSE.md for details.
 */

#include "unit_revfx.h"
#include "frame.h"
#include "reverb.h"

#include <cstdint>

namespace {

// Delay memory size in float words for the local reverb adapter.
constexpr uint32_t k_reverb_buffer_words = 16384;

static clouds::Reverb241A2A9 s_processor_instance;
static float* s_buffer = nullptr;
static float s_reverb_amount = 0.f;
static float s_tone = 1.f;

}  // namespace

extern "C" {

// Request delay memory through unit.cc so large buffers live in SDRAM.
uint32_t _hook_buffer_size() { return k_reverb_buffer_words; }
void _hook_set_buffer(float* buf) { s_buffer = buf; }

void *__dso_handle;

void _hook_init(uint32_t platform, uint32_t api, uint32_t samplerate)
{
  (void)platform;
  (void)api;
  // The Clouds-style core uses a fixed 32 kHz LFO normalization.
  // Keep runtime samplerate out of this path to preserve the Clouds voicing.
  (void)samplerate;
  s_processor_instance.Init(s_buffer);
}

void _hook_process(float *xn, uint32_t frames)
{
  // Keep behavior like Clouds: one reverb knob controls wet signal.
  // Here, DEPTH (Knob B) drives the reverb amount inside the core.
  // TONE controls the low-pass damping in the reverb network.
  // No second wet/dry mix stage is applied after the core.
  // At Knob B = 100%, amount = 0.54, which is about 54% wet / 46% dry.
  // This keeps behavior close to the original one-knob Clouds feel.
  s_processor_instance.set_amount(s_reverb_amount * 0.54f);
  s_processor_instance.set_diffusion(0.7f);
  s_processor_instance.set_time(0.35f + 0.63f * s_reverb_amount);
  s_processor_instance.set_input_gain(0.2f);
  s_processor_instance.set_lp(0.6f + 0.37f * s_tone);

  clouds::FloatFrame* out = reinterpret_cast<clouds::FloatFrame*>(xn);
  s_processor_instance.Process(out, frames);
}

void _hook_param(uint8_t index, int32_t value)
{
  const float value_f = q31_to_f32(value);
  switch (index) {
  case 0:  // TONE: maps to low-pass damping.
    s_tone = value_f;
    break;
  case 1:  // DEPTH (Knob B): maps to internal reverb amount.
    s_reverb_amount = value_f;
    break;
  case 3:  // SHIFT-DEPTH / MIX is off on purpose.
    // Reason: this matches the Clouds one-knob reverb behavior.
    // A separate wet/dry control can be enabled in this file if needed.
    break;
  default:
    break;
  }
}

}  // extern "C"
