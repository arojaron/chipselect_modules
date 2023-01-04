#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct OnePole {
    T Ts;
    T FSp2;

    T z = T(0);
    T a = T(1);
    T b = T(0);

    OnePole(float FS) : Ts(T(1/FS)), FSp2(T(FS/2)) {}

    void setFrequency(T freq)
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        freq = simd::ifelse(freq >= FSp2, FSp2, freq);

        T dfreq = freq*Ts;

        b = simd::exp(dfreq*T(-2.0*M_PI));
        a = T(1)-b;
    }
    T process(T in)
    {
        z = in*a + z*b;
        return z;
    }
};

}
