#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct HighShelf {
    T period_limit;
    T f_m = T(0.9f);
    T phi_m = T(1.f) - simd::cos(T(M_PI*0.9f));

    T in1 = T(0.f);
    T z = T(0.f);
    T a = T(0.f);
    T b0 = T(1.f);
    T b1 = T(0.f);

    HighShelf(float FS) : period_limit(T(2/FS)) {}
    
    void setParams(T freq, T gain)
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        gain = simd::ifelse(gain <= T(0.0001f), T(0.0001f), gain);

        T f_c = freq * period_limit;
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

template <typename T>
struct Tilting{
    T period_limit;
    T pi = T(M_PI);

    T in1 = T(0.f);
    T z = T(0.f);
    T a1 = T(0.f);
    T b0 = T(1.f);
    T b1 = T(0.f);

    Tilting(float FS) : period_limit(T(2/FS)) {}
    
    void setParams(T f, T G)
    {
        f = simd::ifelse(f <= T(0.f), -f, f);
        f = simd::ifelse(f <= T(1.f), T(1.f), f);
        G = simd::ifelse(G <= T(0.0001f), T(0.0001f), G);

        T f_c = f * period_limit;

        a1 = (f_c*f_c*pi*pi - T(4.f)*f_c*simd::sqrt(f_c*f_c + G*G)*pi + T(4.f)*f_c*f_c + T(4.f)*G*G) 
           / (f_c*f_c*pi*pi - T(4.f)*f_c*f_c - T(4.f)*G*G);
            
        T b0pb1 = G*(T(1.f)+a1);
        T b0mb1 = simd::sqrt((T(1.f)-a1)*(T(1.f)-a1)*(G + T(1.f)/(G*f_c*f_c))/(T(1.f)/G + G/(f_c*f_c)));
        b0 = (b0pb1 + b0mb1)*T(0.5f);
        b1 = b0pb1 - b0;
    }

    T process(T in)
    {
        z = b0*in + b1*in1 - a1*z;
        in1 = in;
        return z;
    }
};

template <typename T>
struct TwoShelves{
    T period_limit;
    T pi = T(M_PI);

    T in1 = T(0.f);
    T z = T(0.f);
    T a1 = T(0.f);
    T b0 = T(1.f);
    T b1 = T(0.f);

    TwoShelves(float FS) : period_limit(T(2/FS)) {}
    
    void setParams(T f, T G0, T G1)
    {
        f = simd::ifelse(f <= T(0.f), -f, f);
        f = simd::ifelse(f <= T(1.f), T(1.f), f);
        G0 = simd::ifelse(G0 <= T(0.0001f), T(0.0001f), G0);
        G1 = simd::ifelse(G1 <= T(0.0001f), T(0.0001f), G1);

        T f_c = f * period_limit;

        a1 = (G1*f_c*f_c*pi*pi - T(4.f)*f_c*simd::sqrt(G1*G1*f_c*f_c*pi*pi + G0*G1*pi*pi) + T(4.f)*G1*f_c*f_c + T(4.f)*G0) 
           / (G1*f_c*f_c*pi*pi - T(4.f)*G1*f_c*f_c - T(4.f)*G0);
            
        T b0pb1 = G0*(T(1.f)+a1);
        T b0mb1 = simd::sqrt((T(1.f)-a1)*(T(1.f)-a1)*(G0 + G1/(f_c*f_c))/(T(1.f)/G0 + T(1.f)/(G1*f_c*f_c)));
        b0 = (b0pb1 + b0mb1)*T(0.5f);
        b1 = b0pb1 - b0;
    }

    T process(T in)
    {
        z = b0*in + b1*in1 - a1*z;
        in1 = in;
        return z;
    }

    T getActualHighGain(void){
        return (b0-b1)/(T(1.f)-a1);
    }
};

}