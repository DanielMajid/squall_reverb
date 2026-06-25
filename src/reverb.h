// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (pichenettes@mutable-instruments.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Reverb core that follows the original Clouds DSP structure.
// Keep this file behavior-stable; adapt platform/runtime behavior in
// src/clouds_reverb.cc instead.

#ifndef SQUALL_REVERB_H_
#define SQUALL_REVERB_H_

#include "stmlib/stmlib.h"
#include "fx_engine.h"

namespace clouds {

class Reverb241A2A9 {
 public:
  Reverb241A2A9() { }
  ~Reverb241A2A9() { }

  void Init(float* buffer) {
    engine_.Init(buffer);
    engine_.SetLFOFrequency(LFO_1, 0.5f / 32000.0f);
    engine_.SetLFOFrequency(LFO_2, 0.3f / 32000.0f);
    lp_ = 0.7f;
    diffusion_ = 0.625f;
    // Initialize filter memory so first rendered tails are deterministic.
    lp_decay_1_ = 0.0f;
    lp_decay_2_ = 0.0f;
  }

  void Process(FloatFrame* in_out, size_t size) {
    // Fixed delay topology and taps retained to preserve established voicing.
    typedef E::Reserve<113,
      E::Reserve<162,
      E::Reserve<241,
      E::Reserve<399,
      E::Reserve<1653,
      E::Reserve<2038,
      E::Reserve<3411,
      E::Reserve<1913,
      E::Reserve<1663,
      E::Reserve<4782> > > > > > > > > > Memory;
    E::DelayLine<Memory, 0> ap1;
    E::DelayLine<Memory, 1> ap2;
    E::DelayLine<Memory, 2> ap3;
    E::DelayLine<Memory, 3> ap4;
    E::DelayLine<Memory, 4> dap1a;
    E::DelayLine<Memory, 5> dap1b;
    E::DelayLine<Memory, 6> del1;
    E::DelayLine<Memory, 7> dap2a;
    E::DelayLine<Memory, 8> dap2b;
    E::DelayLine<Memory, 9> del2;
    E::Context c;

    const float kap = diffusion_;
    const float klp = lp_;
    const float krt = reverb_time_;
    const float amount = amount_;
    const float gain = input_gain_;

    float lp_1 = lp_decay_1_;
    float lp_2 = lp_decay_2_;

    while (size--) {
      float wet;
      float apout = 0.0f;
      engine_.Start(&c);

      c.Interpolate(ap1, 10.0f, LFO_1, 60.0f, 1.0f);
      c.Write(ap1, 100, 0.0f);

      c.Read(in_out->l + in_out->r, gain);

      c.Read(ap1 TAIL, kap);
      c.WriteAllPass(ap1, -kap);
      c.Read(ap2 TAIL, kap);
      c.WriteAllPass(ap2, -kap);
      c.Read(ap3 TAIL, kap);
      c.WriteAllPass(ap3, -kap);
      c.Read(ap4 TAIL, kap);
      c.WriteAllPass(ap4, -kap);
      c.Write(apout);

      c.Load(apout);
      c.Interpolate(del2, 4680.0f, LFO_2, 100.0f, krt);
      c.Lp(lp_1, klp);
      c.Read(dap1a TAIL, -kap);
      c.WriteAllPass(dap1a, kap);
      c.Read(dap1b TAIL, kap);
      c.WriteAllPass(dap1b, -kap);
      c.Write(del1, 2.0f);
      c.Write(wet, 0.0f);

      in_out->l += (wet - in_out->l) * amount;

      c.Load(apout);
      c.Read(del1 TAIL, krt);
      c.Lp(lp_2, klp);
      c.Read(dap2a TAIL, kap);
      c.WriteAllPass(dap2a, -kap);
      c.Read(dap2b TAIL, -kap);
      c.WriteAllPass(dap2b, kap);
      c.Write(del2, 2.0f);
      c.Write(wet, 0.0f);

      in_out->r += (wet - in_out->r) * amount;

      ++in_out;
    }

    lp_decay_1_ = lp_1;
    lp_decay_2_ = lp_2;
  }

  inline void set_amount(float amount) {
    amount_ = amount;
  }

  inline void set_input_gain(float input_gain) {
    input_gain_ = input_gain;
  }

  inline void set_time(float reverb_time) {
    reverb_time_ = reverb_time;
  }

  inline void set_diffusion(float diffusion) {
    diffusion_ = diffusion;
  }

  inline void set_lp(float lp) {
    lp_ = lp;
  }

 private:
  typedef FxEngine<16384, FORMAT_32_BIT> E;
  E engine_;

  float amount_;
  float input_gain_;
  float reverb_time_;
  float diffusion_;
  float lp_;

  float lp_decay_1_;
  float lp_decay_2_;

  DISALLOW_COPY_AND_ASSIGN(Reverb241A2A9);
};

}  // namespace clouds

#endif  // SQUALL_REVERB_H_
