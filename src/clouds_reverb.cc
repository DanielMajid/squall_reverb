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

#include <algorithm>
#include <cstdint>

namespace {

// Reverb working memory in float words.
// This is the same single buffer the Clouds-style reverb core expects.
constexpr uint32_t k_reverb_buffer_words = 16384;

// Parameter indices consumed by _hook_param.
// These match the IDs forwarded from unit.cc.
constexpr uint8_t k_param_tone = 0;
constexpr uint8_t k_param_depth = 1;
constexpr uint8_t k_param_mix_passthrough = 2;
constexpr uint8_t k_param_freeze = 3;
constexpr uint8_t k_param_freeze_scan = 4;

// Reverb core scaling constants.
// These keep the broad voicing close to Clouds-era behavior.
constexpr float k_amount_scale = 0.54f;
constexpr float k_time_base = 0.35f;
constexpr float k_time_scale = 0.63f;
constexpr float k_lp_base = 0.6f;
constexpr float k_lp_scale = 0.37f;
constexpr float k_input_gain_base = 0.2f;

// Freeze and scan response constants.
// Smoothing avoids zipper noise from abrupt UI parameter moves.
constexpr float k_freeze_smoothing = 0.04f;
constexpr float k_scan_smoothing = 0.015f;
// Maximum leakage while frozen when SCAN is fully counter-clockwise.
// 0.05 means up to 5% of normal decay/input is allowed.
constexpr float k_freeze_leak_max = 0.05f;

static clouds::Reverb241A2A9 s_processor_instance;
static float* s_reverb_buffer = nullptr;

// Runtime control state. Values are normalized to 0..1 where possible.
static float s_reverb_amount = 0.f;
static float s_tone = 1.f;
static float s_freeze_scan = 0.f;
static bool s_freeze = false;
static float s_freeze_blend = 0.f;
static float s_scan_smoothed = 0.f;

inline float clamp01(const float x) {
  return std::max(0.0f, std::min(1.0f, x));
}

}  // namespace

extern "C" {

// Request delay memory through unit.cc so large buffers live in SDRAM.
uint32_t _hook_buffer_size() { return k_reverb_buffer_words; }
void _hook_set_buffer(float* buf) {
  s_reverb_buffer = buf;
}

void *__dso_handle;

void _hook_init(uint32_t platform, uint32_t api, uint32_t samplerate)
{
  (void)platform;
  (void)api;
  // The reverb core runs with fixed internal normalization.
  // The runtime sample rate is not used by this bridge.
  (void)samplerate;

  // Reset runtime state so loading the unit always starts predictably.
  s_processor_instance.Init(s_reverb_buffer);
  s_reverb_amount = 0.f;
  s_tone = 1.f;
  // FREEZE defaults off at startup.
  s_freeze_scan = 0.f;
  s_freeze = false;
  s_freeze_blend = 0.f;
  // SCAN is smoothed internally; start at zeroed state.
  s_scan_smoothed = 0.f;
}

void _hook_process(float* xn, uint32_t frames)
{
  // Soften hard state changes from the UI for smoother audio transitions.
  const float freeze_target = s_freeze ? 1.0f : 0.0f;
  ONE_POLE(s_freeze_blend, freeze_target, k_freeze_smoothing);
  ONE_POLE(s_scan_smoothed, clamp01(s_freeze_scan), k_scan_smoothing);

  // Base values come from panel controls before freeze/scan modifiers.
  const float base_amount = s_reverb_amount * k_amount_scale;
  const float base_time = k_time_base + k_time_scale * s_reverb_amount;
  const float base_lp = k_lp_base + k_lp_scale * s_tone;

  // SCAN inversely controls a small freeze leakage margin.
  // SCAN 100%: 0% leak (tightest hold).
  // SCAN 0%:   5% leak (slight decay + slight new input).
  const float freeze_leak = (1.0f - s_scan_smoothed) * k_freeze_leak_max;
  // Hold controls how much the tail resists decaying.
  const float freeze_hold = s_freeze_blend * (1.0f - freeze_leak);
  // Input scale controls how much fresh input can still enter while frozen.
  const float input_scale =
      (1.0f - s_freeze_blend) + (s_freeze_blend * freeze_leak);

  // Scan is shaped around center so small moves near noon feel gentler,
  // and extreme knob positions are more dramatic.
  const float scan_bipolar = (s_scan_smoothed * 2.0f - 1.0f);
  const float scan_shaped = scan_bipolar * scan_bipolar * scan_bipolar;
  // Scan color is only active when freeze is active.
  const float scan_amount = scan_shaped * s_freeze_blend;

  // Reverb amount and time are driven by freeze hold, not directly by SCAN.
  const float amount = base_amount + (1.0f - base_amount) * freeze_hold;
  // Keep time tied to hold so SCAN cannot directly "unfreeze" the tail.
  const float reverb_time = base_time + (1.0f - base_time) * freeze_hold;

  const float input_gain = k_input_gain_base * input_scale;
  // SCAN colors the frozen texture by moving damping and diffusion.
  float lp = base_lp + (1.0f - base_lp) * freeze_hold;
  lp += 0.18f * scan_amount;
  lp = clamp01(lp);

  float diffusion = 0.7f + 0.10f * scan_amount;
  diffusion = clamp01(diffusion);

  s_processor_instance.set_amount(amount);
  s_processor_instance.set_diffusion(diffusion);
  s_processor_instance.set_time(reverb_time);
  s_processor_instance.set_input_gain(input_gain);
  s_processor_instance.set_lp(lp);

  // Process in place: xn already contains interleaved stereo input samples.
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
    // Intentionally unused right now.
    break;
  case k_param_freeze:  // Freeze switch.
    s_freeze = value_f >= 0.5f;
    break;
  case k_param_freeze_scan:  // Freeze scan position.
    s_freeze_scan = clamp01(value_f);
    break;
  default:
    break;
  }
}

}  // extern "C"
