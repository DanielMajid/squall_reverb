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

clouds::Reverb reverb;

static float* buffer = nullptr;
extern "C" {

// Request 16384 float words from unit_init() via weak-hook bridge.
uint32_t _hook_buffer_size() { return 16384; }
void _hook_set_buffer(float* buf) { buffer = buf; }
static float reverb_amount = 0.f;
static float cutoff = 1.f;

void *__dso_handle;

void _hook_init(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
  reverb.Init(buffer);
}

void _hook_process(float *xn, uint32_t frames)
{
  clouds::FloatFrame *out = reinterpret_cast<clouds::FloatFrame*>(xn);

  reverb.set_amount(reverb_amount * 0.54f);
  reverb.set_diffusion(0.7f);
  reverb.set_time(0.35f + 0.63f * reverb_amount);
  reverb.set_input_gain(0.2f);
  reverb.set_lp(0.6f + 0.37f * cutoff);
  reverb.Process(out, frames);
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
  case 3:  // SHIFT-DEPTH (unused by this algorithm)
    break;
  default:
    break;
  }
}

}  // extern "C"
