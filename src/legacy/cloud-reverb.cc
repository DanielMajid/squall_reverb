/*
 * File: cloud-reverb.cc
 *
 * NTS-1 MKII Reverb Effect Implementation
 *
 * This file initializes the reverb effect, processes audio frames,
 * and handles parameter changes for the NTS-1 MKII platform.
 *
 *
 */

#include "unit_revfx.h"
#include "frame.h"
#include "reverb.h"

#include <algorithm>

clouds::Reverb reverb;

static float* buffer = nullptr;
extern "C" {

// Request 16384 float words from unit_init() via weak-hook bridge.
uint32_t _hook_buffer_size() { return 16384; }
void _hook_set_buffer(float* buf) { buffer = buf; }
static float reverb_amount = 0.f;
static float cutoff = 1.f;
static float drywet = 1.f;

void *__dso_handle;

void _hook_init(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  reverb.Init(buffer);
}

void _hook_process(float *xn, uint32_t frames)
{
  // Save dry signal before reverb overwrites the buffer.
  // 128 stereo frames is a safe upper bound for the NTS-1 mkII block size.
  float dry[256];
  const uint32_t nsamples = 2 * frames;
  std::copy(xn, xn + nsamples, dry);

  clouds::FloatFrame *out = reinterpret_cast<clouds::FloatFrame*>(xn);

  reverb.set_amount(reverb_amount * 0.54f);
  reverb.set_diffusion(0.7f);
  reverb.set_time(0.35f + 0.63f * reverb_amount);
  reverb.set_input_gain(0.2f);
  reverb.set_lp(0.6f + 0.37f * cutoff);
  reverb.Process(out, frames);

  // Blend dry and wet according to the MIX parameter.
  const float wet = drywet;
  const float dry_gain = 1.f - wet;
  for (uint32_t i = 0; i < nsamples; ++i) {
    xn[i] = dry_gain * dry[i] + wet * xn[i];
  }
}

void _hook_param(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case 0:  // TIME
    cutoff = valf;
    break;
  case 1:  // DEPTH
    reverb_amount = valf;
    break;
  case 3:  // SHIFT-DEPTH (wet/dry mix)
    drywet = valf;
    break;
  default:
    break;
  }
}

}  // extern "C"
