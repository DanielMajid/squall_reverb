#include <algorithm>
#include <cstdint>

#include "unit_revfx.h"
#include "utils/int_math.h"

extern "C" {

// Legacy ported code parameter enum.
enum {
  k_user_revfx_param_time = 0,
  k_user_revfx_param_depth,
  k_user_revfx_param_reserved0,
  k_user_revfx_param_shift_depth,
};

// Weak symbol hooks: override with legacy implementation to enable DSP.
// If no override is provided, they are no-ops and the effect produces silence.

__attribute__((weak)) void _hook_init(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;
}

__attribute__((weak)) void _hook_process(float *in_out, uint32_t frames) {
  (void)in_out;
  (void)frames;
}

__attribute__((weak)) void _hook_suspend(void) {}
__attribute__((weak)) void _hook_resume(void) {}

__attribute__((weak)) void _hook_param(uint8_t index, int32_t value) {
  (void)index;
  (void)value;
}

// Buffer management hooks: legacy DSP reports required SDRAM words,
// then receives the runtime-allocated pointer via _hook_set_buffer.
__attribute__((weak)) uint32_t _hook_buffer_size() { return 0; }
__attribute__((weak)) void _hook_set_buffer(float* buf) { (void)buf; }

}  // extern "C"

namespace {

int32_t cached_values[UNIT_REVFX_MAX_PARAM_COUNT] = {};

inline int32_t drywet_to_shift_depth(const int32_t drywet) {
  const int32_t clamped = clipminmaxi32(-1000, drywet, 1000);
  return (clamped + 1000) * 1023 / 2000;
}

// The Clouds reverb amount also stretches decay time. Blend a mostly linear
// response with a little quadratic taper so the lower half moves more while
// the top of the knob still keeps extra resolution for the longest tails.
inline int32_t depth_to_reverb_amount(const int32_t depth) {
  const int32_t clamped = clipminmaxi32(0, depth, 1023);
  const float normalized = static_cast<float>(clamped) / 1023.f;
  const float curved = normalized * (0.65f + 0.35f * normalized);
  return static_cast<int32_t>(curved * 1023.f + 0.5f);
}

// Maps mkII parameter range [0, 1023] to legacy q31 range [0, 0x7FFFFFFF].
// Use when forwarding to legacy code that calls q31_to_f32(value).
inline int32_t legacy_param_to_q31(int32_t value) {
  const int32_t clamped = clipminmaxi32(0, value, 1023);
  return static_cast<int32_t>((static_cast<int64_t>(clamped) * 0x7FFFFFFFLL) / 1023);
}

}  // namespace

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc) {
  if (!desc) {
    return k_unit_err_undef;
  }

  if (desc->target != unit_header.target) {
    return k_unit_err_target;
  }

  if (!UNIT_API_IS_COMPAT(desc->api)) {
    return k_unit_err_api_version;
  }

  if (desc->input_channels != 2 || desc->output_channels != 2) {
    return k_unit_err_geometry;
  }

  // Allocate runtime SDRAM when legacy DSP needs delay memory.
  // Keeping large buffers out of static data helps stay within unit size limits.
  const uint32_t buf_words = _hook_buffer_size();
  if (buf_words > 0) {
    if (!desc->hooks.sdram_alloc) {
      return k_unit_err_memory;
    }
    float* buf = reinterpret_cast<float*>(
      desc->hooks.sdram_alloc(buf_words * sizeof(float)));
    if (!buf) {
      return k_unit_err_memory;
    }
    // Start from deterministic silence. sdram_alloc() does not guarantee
    // contents, and delay memories with stale values can produce audible artifacts.
    std::fill_n(buf, buf_words, 0.f);
    _hook_set_buffer(buf);
  }

  // Initialize legacy DSP code.
  _hook_init(desc->target, desc->api);

  // Initialize parameters to their default values.
  for (uint8_t index = 0; index < UNIT_REVFX_MAX_PARAM_COUNT; ++index) {
    cached_values[index] = static_cast<int32_t>(unit_header.params[index].init);
    unit_set_param_value(index, cached_values[index]);
  }

  return k_unit_err_none;
}

__unit_callback void unit_teardown() {}

__unit_callback void unit_reset() {}

__unit_callback void unit_resume() {
  _hook_resume();
}

__unit_callback void unit_suspend() {
  _hook_suspend();
}

__unit_callback void unit_render(const float *in, float *out, uint32_t frames) {
  std::copy(in, in + (frames << 1), out);
  _hook_process(out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  if (id >= UNIT_REVFX_MAX_PARAM_COUNT) {
    return;
  }

  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
  cached_values[id] = value;

  switch (id) {
  case k_unit_revfx_fixed_param_time:
    _hook_param(k_user_revfx_param_time, legacy_param_to_q31(value));
    break;

  case k_unit_revfx_fixed_param_depth:
    _hook_param(k_user_revfx_param_depth, legacy_param_to_q31(depth_to_reverb_amount(value)));
    break;

  case k_unit_revfx_fixed_param_mix:
    _hook_param(k_user_revfx_param_shift_depth, legacy_param_to_q31(drywet_to_shift_depth(value)));
    break;

  default:
    break;
  }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  if (id >= UNIT_REVFX_MAX_PARAM_COUNT) {
    return 0;
  }

  return cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
  return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  (void)tempo;
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  (void)counter;
}
