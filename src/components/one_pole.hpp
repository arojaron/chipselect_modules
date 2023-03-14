#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct OnePole {
    T Ts;
    T FSp2;

    T z = T(0);
    T b = T(1);
    T a = T(0);

    OnePole(float FS) : Ts(T(1/FS)), FSp2(T(FS/2)) {}

    void setFrequency(T freq)
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        freq = simd::ifelse(freq >= FSp2, FSp2, freq);

        T dfreq = freq*Ts;

        a = simd::exp(dfreq*T(-2.0*M_PI));
        b = T(1)-a;
    }
    T process(T in)
    {
        z = in*b + z*a;
        return z;
    }
};
}
