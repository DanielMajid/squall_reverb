#include <algorithm>
#include <cstdint>

#include "unit_revfx.h"
#include "utils/int_math.h"

extern "C" {

// Bridge parameter IDs consumed by _hook_param in clouds_reverb.cc.
// Index 0 is named TONE in this project (SDK slot k_unit_revfx_fixed_param_time).
enum {
  k_user_revfx_param_tone = 0,
  k_user_revfx_param_depth,
  k_user_revfx_param_mix_passthrough,
  k_user_revfx_param_freeze,
};

constexpr uint8_t k_unit_revfx_param_freeze_slot = 3;

// DSP bridge hooks implemented by src/clouds_reverb.cc.
// Keep these as required symbols so mismatches fail at link time.
void _hook_init(uint32_t platform, uint32_t api, uint32_t samplerate);
void _hook_process(float *in_out, uint32_t frames);
void _hook_suspend(void);
void _hook_resume(void);
void _hook_param(uint8_t index, int32_t value);

// Buffer management hooks: DSP reports required SDRAM words,
// then receives the runtime-allocated pointer via _hook_set_buffer.
uint32_t _hook_buffer_size();
void _hook_set_buffer(float* buf);

}  // extern "C"

namespace {

int32_t cached_values[UNIT_REVFX_MAX_PARAM_COUNT] = {};

inline int32_t drywet_to_shift_depth(const int32_t drywet) {
  // Keep MIX conversion in place for compatibility.
  const int32_t clamped = clipminmaxi32(-1000, drywet, 1000);
  return (clamped + 1000) * 1023 / 2000;
}

// Maps mkII parameter range [0, 1023] to q31 range [0, 0x7FFFFFFF].
// Use when forwarding to the processor bridge that calls q31_to_f32(value).
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

  // Initialize DSP processor bridge.
  _hook_init(desc->target, desc->api, desc->samplerate);

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
  // Process in-place on the output buffer so the bridge can mutate stereo frames.
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
    // SDK fixed slot "time" is used as TONE for this effect.
    _hook_param(k_user_revfx_param_tone, legacy_param_to_q31(value));
    break;

  case k_unit_revfx_fixed_param_depth:
    _hook_param(k_user_revfx_param_depth, legacy_param_to_q31(value));
    break;

  case k_unit_revfx_fixed_param_mix:
    // MIX remains in its original slot but is intentionally not used.
    // Forward to a dedicated passthrough index so behavior stays explicit.
    _hook_param(
        k_user_revfx_param_mix_passthrough,
        legacy_param_to_q31(drywet_to_shift_depth(value)));
    break;

  case k_unit_revfx_param_freeze_slot:
    // FREEZE at the next available slot, mapped as a unipolar 0..1 control.
    _hook_param(k_user_revfx_param_freeze, legacy_param_to_q31(value));
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
