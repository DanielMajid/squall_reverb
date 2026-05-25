/*
 * File: clouds_reverb.cc
 *
 *
 * Calls core DSP from Mutable Instruments Clouds.
 *
 * Clouds DSP Copyright (c) 2014 Emilie Gillet released under GPL3.0 License. See LICENSE.md for details.
 * 
 * 
 */

#include "unit_revfx.h"
#include "frame.h"
#include "reverb.h"

#include <algorithm>
#include <cstdint>

namespace {

// Delay memory size in uint16 samples expected by clouds::Reverb::Init().
constexpr uint32_t k_reverb_buffer_samples_16 = 16384;
constexpr uint32_t k_max_chunk_frames = 128;
constexpr uint32_t k_dry_scratch_samples = 2 * k_max_chunk_frames;

static clouds::Reverb s_processor_instance;
static uint16_t* s_buffer = nullptr;
static float s_dry_scratch[k_dry_scratch_samples];
static float s_reverb_amount = 0.f;
static float s_cutoff = 1.f;
static float s_dry_wet = 1.f;

}  // namespace

extern "C" {

// Request delay memory through unit.cc so large buffers live in SDRAM.
uint32_t _hook_buffer_size() { return (k_reverb_buffer_samples_16 + 1U) / 2U; }
void _hook_set_buffer(float* buf) { s_buffer = reinterpret_cast<uint16_t*>(buf); }

void *__dso_handle;

void _hook_init(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  s_processor_instance.Init(s_buffer);
}

void _hook_process(float *xn, uint32_t frames)
{
  // Keep these coefficients stable to preserve the known-good Squall voicing.
  s_processor_instance.set_amount(s_reverb_amount * 0.54f);
  s_processor_instance.set_diffusion(0.7f);
  s_processor_instance.set_time(0.35f + 0.63f * s_reverb_amount);
  s_processor_instance.set_input_gain(0.2f);
  s_processor_instance.set_lp(0.6f + 0.37f * s_cutoff);

  // Blend dry/wet in bounded chunks to avoid buffer overruns for larger
  // runtime block sizes.
  const float wet = s_dry_wet;
  const float dry_gain = 1.f - wet;
  uint32_t frame_offset = 0;

  while (frame_offset < frames) {
    const uint32_t chunk_frames = std::min(k_max_chunk_frames, frames - frame_offset);
    const uint32_t chunk_samples = 2 * chunk_frames;
    float* chunk = xn + (2 * frame_offset);

    std::copy(chunk, chunk + chunk_samples, s_dry_scratch);

    clouds::FloatFrame* out = reinterpret_cast<clouds::FloatFrame*>(chunk);
    s_processor_instance.Process(out, chunk_frames);

    for (uint32_t i = 0; i < chunk_samples; ++i) {
      chunk[i] = dry_gain * s_dry_scratch[i] + wet * chunk[i];
    }

    frame_offset += chunk_frames;
  }
}

void _hook_param(uint8_t index, int32_t value)
{
  const float value_f = q31_to_f32(value);
  switch (index) {
  case 0:  // TIME
    s_cutoff = value_f;
    break;
  case 1:  // DEPTH
    s_reverb_amount = value_f;
    break;
  case 3:  // SHIFT-DEPTH (wet/dry mix)
    s_dry_wet = value_f;
    break;
  default:
    break;
  }
}

}  // extern "C"
