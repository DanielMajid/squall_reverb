/*
 * File: clouds_reverb.cc
 *
 * NTS-1 mkII reverb bridge for the Clouds reverb core.
 *
 * This file owns runtime hook wiring, parameter forwarding, and delay-memory
 * handoff. DSP behavior is implemented in src/reverb.h.
 *
 * Clouds DSP Copyright (c) 2014 Emilie Gillet released under MIT License. See LICENSE for details.
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
static bool s_freeze = false;
static float s_freeze_blend = 0.f;

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
  s_reverb_amount = 0.f;
  s_tone = 1.f;
  s_freeze = false;
  s_freeze_blend = 0.f;
}

void _hook_process(float *xn, uint32_t frames)
{
  // Freeze behaves like a button: ON holds the current wash, OFF resumes
  // normal feed into the network.
  // Smooth transitions to avoid abrupt tonal or level jumps when toggling.
  const float freeze_target = s_freeze ? 1.0f : 0.0f;
  ONE_POLE(s_freeze_blend, freeze_target, 0.04f);

  const float base_amount = s_reverb_amount * 0.54f;
  const float base_time = 0.35f + 0.63f * s_reverb_amount;
  const float base_lp = 0.6f + 0.37f * s_tone;

  const float amount = base_amount + (1.0f - base_amount) * s_freeze_blend;
  const float reverb_time = base_time;
  const float input_gain = 0.2f * (1.0f - s_freeze_blend);
  const float lp = base_lp + (0.95f - base_lp) * s_freeze_blend;

  s_processor_instance.set_amount(amount);
  s_processor_instance.set_diffusion(0.7f);
  s_processor_instance.set_time(reverb_time);
  s_processor_instance.set_input_gain(input_gain);
  s_processor_instance.set_lp(lp);

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
  case 3:  // FREEZE: button-like thresholded state.
    s_freeze = value_f >= 0.5f;
    break;
  default:
    break;
  }
}

}  // extern "C"
