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

// Parameter indices consumed by _hook_param.
constexpr uint8_t k_param_tone = 0;
constexpr uint8_t k_param_depth = 1;
constexpr uint8_t k_param_mix_passthrough = 2;
constexpr uint8_t k_param_freeze = 3;
constexpr uint8_t k_param_freeze_timbre = 4;

// Reverb core scaling constants.
constexpr float k_amount_scale = 0.54f;
constexpr float k_time_base = 0.35f;
constexpr float k_time_scale = 0.63f;
constexpr float k_lp_base = 0.6f;
constexpr float k_lp_scale = 0.37f;
constexpr float k_input_gain_base = 0.2f;

// Freeze shaping constants.
constexpr float k_freeze_smoothing = 0.04f;
constexpr float k_timbre_lock_start = 0.8f;
constexpr float k_timbre_input_fade_start = 0.55f;

static clouds::Reverb241A2A9 s_processor_instance;
static float* s_buffer = nullptr;
static float s_reverb_amount = 0.f;
static float s_tone = 1.f;
static float s_freeze_timbre = 0.f;
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
  // The reverb core runs with fixed internal normalization.
  // The runtime sample rate is not used by this bridge.
  (void)samplerate;

  // Clear runtime state so the effect starts from a known condition.
  s_processor_instance.Init(s_buffer);
  s_reverb_amount = 0.f;
  s_tone = 1.f;
  s_freeze_timbre = 0.f;
  s_freeze = false;
  s_freeze_blend = 0.f;
}

void _hook_process(float* xn, uint32_t frames)
{
  // Blend into and out of freeze instead of switching abruptly.
  const float freeze_target = s_freeze ? 1.0f : 0.0f;
  ONE_POLE(s_freeze_blend, freeze_target, k_freeze_smoothing);

  // Base values come directly from panel controls.
  const float base_amount = s_reverb_amount * k_amount_scale;
  const float base_time = k_time_base + k_time_scale * s_reverb_amount;
  const float base_lp = k_lp_base + k_lp_scale * s_tone;

  const float timbre = clipminf(s_freeze_timbre, 1.0f);

  // Freeze timbre has two zones:
  // 1) loose hold for layering and movement,
  // 2) lock zone near the top for near-endless hold.
  float hold_strength;
  if (timbre < k_timbre_lock_start) {
    const float x = timbre * 1.25f;
    hold_strength = 0.35f + 0.50f * x * x;
  } else {
    const float x = (timbre - k_timbre_lock_start) * 5.0f;
    hold_strength = 0.85f + 0.15f * x * x;
  }
  const float freeze_hold = s_freeze_blend * hold_strength;

  // Keep input available in loose freeze and fade it out near lock.
  float frozen_input_scale = 1.0f;
  if (timbre > k_timbre_input_fade_start) {
    float u = (timbre - k_timbre_input_fade_start) /
        (1.0f - k_timbre_input_fade_start);
    u = clipminf(u, 1.0f);
    // Use a smoothstep curve so the fade feels gradual at both ends.
    const float smooth = u * u * (3.0f - 2.0f * u);
    frozen_input_scale = 1.0f - smooth;
  }
  const float input_scale = (1.0f - s_freeze_blend) + s_freeze_blend * frozen_input_scale;

  // While frozen, timbre steers these values from loose hold to lock.
  const float amount = base_amount + (1.0f - base_amount) * freeze_hold;
  const float reverb_time = base_time + (1.0f - base_time) * freeze_hold;
  const float input_gain = k_input_gain_base * input_scale;
  const float lp = base_lp + (1.0f - base_lp) * freeze_hold;

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
  // q31_to_f32 maps incoming unit values to the 0..1 control range.
  const float value_f = q31_to_f32(value);
  switch (index) {
  case k_param_tone:  // Tail tone / damping.
    s_tone = value_f;
    break;
  case k_param_depth:  // Main reverb depth control.
    s_reverb_amount = value_f;
    break;
  case k_param_mix_passthrough:  // Kept for slot compatibility.
    break;
  case k_param_freeze:  // Freeze switch.
    s_freeze = value_f >= 0.5f;
    break;
  case k_param_freeze_timbre:  // Freeze timbre / hold character.
    s_freeze_timbre = value_f;
    break;
  default:
    break;
  }
}

}  // extern "C"
