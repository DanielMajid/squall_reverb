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
// Maximum fresh-input bleed while frozen when SCAN is fully counter-clockwise.
// 0.05 means up to 5% of normal input is allowed into the frozen tail.
constexpr float k_freeze_input_bleed_max = 0.05f;
// Leak-out from the held tail while SCAN is in the lower half.
// 0.005 means a fixed 0.5% release from the frozen buffer.
constexpr float k_freeze_leak_out_when_scan_open = 0.005f;
// Upper-half SCAN (50-100%) color sweep tuning.
// Keep this restrained to avoid feeding energy back into the tail.
constexpr float k_scan_lp_sweep = 0.24f;
constexpr float k_scan_diffusion_base = 0.50f;
constexpr float k_scan_diffusion_sweep = 0.22f;

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

void _hook_resume(void) {}

void _hook_suspend(void) {}

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

    // SCAN is split into two ranges:
    // - 50% to 100%: fully locked with a full color sweep.
    // - 50% to 0%: increasing buffer leak and fresh-input bleed.
    const float scan_lock = clamp01((s_scan_smoothed - 0.5f) * 2.0f);
    const float scan_leak = clamp01((0.5f - s_scan_smoothed) * 2.0f);

    const float input_bleed = scan_leak * k_freeze_input_bleed_max;
    const float freeze_leak_out = scan_leak > 0.0f
      ? k_freeze_leak_out_when_scan_open
      : 0.0f;
    // Hold controls how much the tail resists decaying.
    const float freeze_hold = s_freeze_blend * (1.0f - freeze_leak_out);
    // Input scale controls how much fresh input can still enter while frozen.
    const float input_scale =
      (1.0f - s_freeze_blend) + (s_freeze_blend * input_bleed);

    // Push the color range harder so the locked half of SCAN feels more dramatic.
    const float scan_color = scan_lock * scan_lock * (0.75f + 0.25f * scan_lock);
    // Scan color is only active when freeze is active.
    const float scan_amount = scan_color * s_freeze_blend;

  // Reverb amount and time are driven by freeze hold, not directly by SCAN.
  const float amount = base_amount + (1.0f - base_amount) * freeze_hold;
  // Keep time tied to hold so SCAN cannot directly "unfreeze" the tail.
  const float reverb_time = base_time + (1.0f - base_time) * freeze_hold;

  // Normalize the frozen tail with a wet-only gain so freeze does not
  // create a large jump in apparent level.
  const float freeze_wet_gain = 1.0f - 0.45f * s_freeze_blend;

  const float input_gain = k_input_gain_base * input_scale;
  // SCAN colors the frozen texture by moving damping and diffusion.
  float lp = base_lp + (1.0f - base_lp) * freeze_hold;
  lp += k_scan_lp_sweep * scan_amount;
  lp = clamp01(lp);

  float diffusion = k_scan_diffusion_base + k_scan_diffusion_sweep * scan_amount;
  diffusion = std::min(diffusion, 0.74f);
  diffusion = clamp01(diffusion);

  s_processor_instance.set_amount(amount);
  s_processor_instance.set_diffusion(diffusion);
  s_processor_instance.set_time(reverb_time);
  s_processor_instance.set_input_gain(input_gain);
  s_processor_instance.set_lp(lp);
  s_processor_instance.set_freeze_wet_gain(freeze_wet_gain);

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
