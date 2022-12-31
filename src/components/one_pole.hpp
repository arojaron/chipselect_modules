#pragma once

#include "rack.hpp"

struct OnePole4 {
    simd::float_4 z = simd::float_4::zero();
    simd::float_4 a = simd::float_4(1);
    simd::float_4 b = simd::float_4::zero();

    void setFrequency(simd::float_4 dfreq)
    {
        b = simd::pow(simd::float_4(M_E), dfreq*simd::float_4(-2.0*M_PI));
        a = simd::float_4(1)-b;
    }
    simd::float_4 process(simd::float_4 in)
    {
        z = in*a + z*b;
        return z;
    }
};