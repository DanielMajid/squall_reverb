/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "unit_revfx.h"
#include <climits>
#include "buffer_ops.h"
#include "reverb.h"

static unit_runtime_desc_t runtime_desc;

enum {
    TIME = 0U,
    DEPTH,
    MIX,
    PARAM4,
    NUM_PARAMS
};

enum {
    PARAM4_VALUE0 = 0,
    PARAM4_VALUE1,
    PARAM4_VALUE2,
    PARAM4_VALUE3,
    NUM_PARAM4_VALUES,
};

static struct {
    int32_t time = 0;
    int32_t depth = 0;
    float mix = 0.f;
    int32_t param4 = 0;
} s_param;

// ---- Callbacks exposed to runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc)
        return k_unit_err_undef;

    if (desc->target != unit_header.target)
        return k_unit_err_target;

    if (!UNIT_API_IS_COMPAT(desc->api))
        return k_unit_err_api_version;

    if (desc->samplerate != 48000)
        return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
        return k_unit_err_geometry;

    if (!desc->hooks.sdram_alloc)
        return k_unit_err_memory;

    // reverb_ram_sampling = (float *)desc->hooks.sdram_alloc(LCW_REVERB_SAMPLING_SIZE * sizeof(float));
    // if (!reverb_ram_sampling)
    //     return k_unit_err_memory;

    // buf_clr_f32(reverb_ram_sampling, LCW_REVERB_SAMPLING_SIZE);

    runtime_desc = *desc;

    // TODO: reset
    s_param.time = 0;
    s_param.depth = 0;
    s_param.mix = 0.f;
    s_param.param4 = 0;

    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1); // output_channels: 2

    // -1.0 .. +1.0 -> 0.0 .. 1.0
    const float wet = (s_param.mix + 1.f) / 2.f;
    const float dry = 1.f - wet;

    for (; out_p != out_e; in_p += 2, out_p += 2) {
        const float xL = *(in_p + 0);
        // const float xR = *(in_p + 1);

        // TODO: Process samples here
        const float wL = xL;
        // const float wR = xR;

        out_p[0] = (dry * xL) + (wet * wL);
        out_p[1] = (dry * xL) + (wet * wL);
        // *(x++) = (dry * xR) + (wet * wR);
    }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    switch (id) {
    case TIME:
        s_param.time = clipminmaxi32(0, value, 1023);
        break;
    case DEPTH:
        s_param.depth = clipminmaxi32(0, value, 1023);
        break;
    case MIX:
        // -100.0 .. 100.0 -> -1.0 .. 1.0
        value = clipminmaxi32(-1000, value, 1000);
        s_param.mix = value / 1000.f;
        break;
    case PARAM4:
        s_param.param4 = value;
        break;
    default:
        break;
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    switch (id) {
    case TIME:
        return s_param.time;
        break;
    case DEPTH:
        return s_param.depth;
        break;
    case MIX:
        // -1.0 .. 1.0 -> -100.0 .. 100.0
        return (int32_t)(s_param.mix * 1000);
        break;
    case PARAM4:
        return s_param.param4;
        break;
    default:
        break;
    }

    return INT_MIN;
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    static const char * param4_strings[NUM_PARAM4_VALUES] = {
        "VAL0",
        "VAL1",
        "VAL2",
        "VAL3",
    };

    switch (id) {
    case PARAM4:
        if (value >= PARAM4_VALUE0 && value < NUM_PARAM4_VALUES)
            return param4_strings[value];
        break;
    default:
        break;
    }

    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}
