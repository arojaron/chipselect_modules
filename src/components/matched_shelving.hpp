#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct HighShelf {
    T FSp2;
    T f_m = T(0.9f);
    T phi_m = T(1.f) - simd::cos(T(M_PI*0.9f));

    T in1 = T(0.f);
    T z = T(0.f);
    T a = T(0.f);
    T b0 = T(1.f);
    T b1 = T(0.f);

    HighShelf(float FS) : FSp2(T(FS/2)) {}
    void setParams(T freq, T gain)
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        gain = simd::ifelse(gain <= T(0.0001f), T(0.0001f), gain);

        T f_c = freq/FSp2;
        T alpha = T(2.f/(M_PI*M_PI))*(T(1.f)/(f_m*f_m) + T(1.f)/(gain*f_c*f_c)) - T(1.f)/phi_m;
        T beta = T(2.f/(M_PI*M_PI))*(T(1.f)/(f_m*f_m) + gain/(f_c*f_c)) - T(1.f)/phi_m;

        a = -alpha / (T(1.f) + alpha + simd::sqrt(T(1.f) + T(2.f)*alpha));
        T b = -beta / (T(1.f) + beta + simd::sqrt(T(1.f) + T(2.f)*beta));

        b0 = (T(1.f) + a) / (T(1.f) + b);
        b1 = b0 * b;
    }

    T process(T in)
    {
        z = b0*in + b1*in1 - a*z;
        in1 = in;
        return z;
    }
};

}