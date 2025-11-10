/*
Copyright 2025 Daniel Mirzakhani
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "unit_revfx.h"
#include <climits>
#include "reverb.h"
#include "frame.h"

// Declare a global instance of the Reverb class
static clouds::Reverb reverb;

#define k_user_revfx_param_time 0
#define k_user_revfx_param_depth 1
#define k_user_revfx_param_shift_depth 2

static unit_runtime_desc_t s_desc;
static int32_t p_[11];

#define MAX_DEPTH 0.99f

static float s_depth;
static float s_time;

typedef struct delay_buffer_t {
    int length;
    int pos;
    float gain;
    float *buf;
} delay_buffer_t;

inline void delay_setmem(delay_buffer_t *delay, int length, float* mem) {
    delay->buf = mem;
    delay->length = length;
    delay->pos = 0;
    delay->gain = 0.7;
}

inline void delay_add(delay_buffer_t *delay, float data) {
    delay->buf[delay->pos++] = data;
    if (delay->pos == delay->length) {
        delay->pos = 0;
    }
}

inline float delay_read_write(delay_buffer_t *delay, float data){
    float buf_in;
    float buf_out;

    buf_out = delay->buf[delay->pos];
    delay_add(delay, data + delay->gain * buf_out);
    return buf_out;
}

#define DELAYap1_LEN 113
#define DELAYap1_GAIN 0.871402
static delay_buffer_t delayap1;
//static __sdram float delay0_mem[DELAY1_LEN] = {0.f};
static float *delayap1_mem;

#define DELAYap2_LEN 162
#define DELAYap2_GAIN 0.882762
static delay_buffer_t delayap2;
//static __sdram float delay1_mem[DELAY2_LEN] = {0.f};
static float *delayap2_mem;

#define DELAYap3_LEN 241
#define DELAYap3_GAIN 0.891443
static delay_buffer_t delayap3;
//static __sdram float delay2_mem[DELAY3_LEN] = {0.f};
static float *delayap3_mem;

#define DELAYap4_LEN 399
#define DELAYap4_GAIN 0.901117
static delay_buffer_t delayap4;
//static __sdram float delay3_mem[DELAY4_LEN] = {0.f};
static float *delayap4_mem;

#define DELAY4dapla_LEN 1653
#define DELAYdapla_GAIN 0.901117
static delay_buffer_t delaydap1a;
//static __sdram float delay4_mem[DELAY4_LEN] = {0.f};
static float *delaydap1a_mem;

#define DELAYdap1b_LEN 2038
#define DELAYdap1b_GAIN 0.901117
static delay_buffer_t delaydap1b;
//static __sdram float delay5_mem[DELAY4_LEN] = {0.f};
static float *delaydap1b_mem;

#define DELAY1_LEN 3411
#define DELAY1_GAIN 0.901117
static delay_buffer_t delay1;
//static __sdram float delay6_mem[DELAY4_LEN] = {0.f};
static float *delay1_mem;

#define DELAYdap2a_LEN 1913
#define DELAYdap2a_GAIN 0.901117
static delay_buffer_t delaydap2a;
//static __sdram float delay7_mem[DELAY4_LEN] = {0.f};
static float *delaydap2a_mem;

#define DELAYdap2b_LEN 1663
#define DELAYdap2b_GAIN 0.901117
static delay_buffer_t delaydap2b;
//static __sdram float delay8_mem[DELAY4_LEN] = {0.f};
static float *delaydap2b_mem;

#define DELAYdel2_LEN 4782
#define DELAYdel2_GAIN 0.901117
static delay_buffer_t delaydel2;
//static __sdram float delay4_mem[DELAY4_LEN] = {0.f};
static float *delaydel2_mem;

inline float allpass_read_write(delay_buffer_t *allpass, float data){
    float buf_in;
    float buf_out;

    buf_out = allpass->buf[allpass->pos];
    buf_in = data + allpass->gain * buf_out;
    delay_add(allpass, buf_in);
    return (buf_out - allpass->gain * buf_in);
}

#define ALLPASS1_LEN 241
#define ALLPASS1_GAIN 0.7
static delay_buffer_t allpass1;
//static __sdram float allpass1_mem[ALLPASS1_LEN] = {0.f};
static float *allpass1_mem;

#define ALLPASS2_LEN 83
#define ALLPASS2_GAIN 0.7
static delay_buffer_t allpass2;
//static __sdram float allpass2_mem[ALLPASS2_LEN] = {0.f};
static float *allpass2_mem;

inline float *sdram_alloc_f32(size_t bufsize) {
    float *m = (float *)s_desc.hooks.sdram_alloc(bufsize * sizeof(float) + 32);
    if (m) {
        buf_clr_f32(m, bufsize);
    }
    return m;
}

inline void sdram_free(void *buf) {
    s_desc.hooks.sdram_free((uint8_t *)buf);
}

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)
{
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    
    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->input_channels != 2 || desc->output_channels != 2)  // should be stereo input/output
      return k_unit_err_geometry;

    // Cache the runtime descriptor for later use
    s_desc = *desc;

    if (!desc->hooks.sdram_alloc)
        return k_unit_err_memory;

    // allocate buffers on SDRAM
    delay1_mem = sdram_alloc_f32(DELAY1_LEN);
    if (delay1_mem) {
        delay_setmem(&delay1, DELAY1_LEN, delay1_mem);
    } else {
        return k_unit_err_memory;
    }
    delay2_mem = sdram_alloc_f32(DELAY2_LEN);
    if (delay2_mem) {
        delay_setmem(&delay2, DELAY2_LEN, delay2_mem);
    } else {
        return k_unit_err_memory;
    }
    delay3_mem = sdram_alloc_f32(DELAY3_LEN);
    if (delay3_mem) {
        delay_setmem(&delay3, DELAY3_LEN, delay3_mem);
    } else {
        return k_unit_err_memory;
    }
    delay4_mem = sdram_alloc_f32(DELAY4_LEN);
    if (delay4_mem) {
        delay_setmem(&delay4, DELAY4_LEN, delay4_mem);
    } else {
        return k_unit_err_memory;
    }
    allpass1_mem = sdram_alloc_f32(ALLPASS1_LEN);
    if (allpass1_mem) {
        delay_setmem(&allpass1, ALLPASS1_LEN, allpass1_mem);
    } else {
        return k_unit_err_memory;
    }
    allpass2_mem = sdram_alloc_f32(ALLPASS2_LEN);
    if (allpass2_mem) {
        delay_setmem(&allpass2, ALLPASS2_LEN, allpass2_mem);
    } else {
        return k_unit_err_memory;
    }
    s_depth = 0.f;

    return k_unit_err_none;
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames)
{
    float * __restrict in_p = (float *) in;
    float * __restrict out_p = out;
    const float * out_e = out_p + 2 * frames;

    for (; out_p != out_e; ) {
        float dry = *in_p++;
        dry = (dry + *in_p++) * 0.5;
        float r;
        float reverb = 0;
        float early = 0;

        reverb += 0.25 * delay_read_write(&delay1, dry);
        reverb += 0.25 * delay_read_write(&delay2, dry);
        reverb += 0.25 * delay_read_write(&delay3, dry);
        reverb += 0.25 * delay_read_write(&delay4, dry);

        reverb = allpass_read_write(&allpass1, reverb);
        reverb = allpass_read_write(&allpass2, reverb);

        *(out_p++) = dry + s_depth * reverb;
        *(out_p++) = dry + s_depth * reverb;
    }
}


__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
    const float valf = param_10bit_to_f32(value);
    switch (id) {
    case k_user_revfx_param_time:
        s_time = valf;
        delay1.gain = DELAY1_GAIN * s_time;
        delay2.gain = DELAY2_GAIN * s_time;
        delay3.gain = DELAY3_GAIN * s_time;
        delay4.gain = DELAY4_GAIN * s_time;
        allpass1.gain = ALLPASS1_GAIN * s_time;
        allpass2.gain = ALLPASS2_GAIN * s_time;
        break;
    case k_user_revfx_param_depth:
        s_depth = valf * MAX_DEPTH;
        break;
    case k_user_revfx_param_shift_depth:
        break;
    default:
        break;
  }
  p_[id] = value;
}

__unit_callback void unit_teardown() {
    sdram_free(delay1_mem);
    sdram_free(delay2_mem);
    sdram_free(delay3_mem);
    sdram_free(delay4_mem);
    sdram_free(allpass1_mem);
    sdram_free(allpass2_mem);
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return p_[id];
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}