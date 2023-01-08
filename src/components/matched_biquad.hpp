/*
Attempt at implementing some of the algorithms in
"Matched Second Order Digital Filter" by Martin Vicanek
https://www.vicanek.de/articles/BiquadFits.pdf
Also utilizing the "complex one-pole" method described in
"Fast Modulation of Filter Parameters" by Ammar Muqaddas
http://www.solostuff.net/wp-content/uploads/2019/05/Fast-Modulation-of-Filter-Parameters-v1.1.1.pdf
*/

#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct LowPass {
    T Ts;
    T Flimit;

    T b[3] = {};
    T alpha = 0;
    T beta = 0;

    T x[3] = {};
    T re1[2] = {};
    T im1[2] = {};
    T re2[2] = {};
    T im2[2] = {};

    LowPass(float FS) : Ts(T(1/FS)), Flimit(T(FS*0.45)) {}

    void setParams(T freq, T Q = T(0.5))
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        freq = simd::ifelse(freq >= Flimit, Flimit, freq);
        Q = simd::ifelse(Q <= 0.5, 0.5, Q);

        T dfreq = freq*Ts;
        T w = T(2*M_PI)*dfreq;
        T q = T(0.5)/Q;

        T R = simd::exp(-q*w);
        T phi = simd::sqrt(1-q*q)*w;
        alpha = R*simd::cos(phi);
        beta = R*simd::sin(phi);

        T a[3] = {T(1),T(0),T(0)};
        a[1] = T(-2)*alpha;
        a[2] = alpha*alpha + beta*beta;

        T f02 = dfreq*dfreq;
        T r0 = a[0] + a[1] + a[2];
        T r1 = f02*(a[0] - a[1] + a[2])/simd::sqrt((1-f02)*(1-f02) + f02/(Q*Q));

        b[0] = T(0.5)*(r0 + r1);
        b[1] = r0 - b[0];
        b[2] = T(0);
    }

    T process(T signal)
    {        
        x[2] = x[1];
        x[1] = x[0];
        re1[1] = re1[0];
        im1[1] = im1[0];
        re2[1] = re2[0];
        im2[1] = im2[0];

        x[0] = signal;
        signal = b[0]*x[0] + b[1]*x[1] + b[2]*x[2];

        re1[0] = signal + alpha*re1[1] - beta*im1[1];
        im1[0] = alpha*im1[1] + beta*re1[1];
        re2[0] = re1[0] + alpha*re2[1];

        return re2[0];
    }
};

template <typename T>
struct HighPass {
    T Ts;
    T Flimit;

    T b[3] = {};
    T alpha = 0;
    T beta = 0;

    T x[3] = {};
    T re1[2] = {};
    T im1[2] = {};
    T re2[2] = {};
    T im2[2] = {};

    HighPass(float FS) : Ts(T(1/FS)), Flimit(T(FS*0.45)) {}

    void setParams(T freq, T Q = T(0.5))
    {
        freq = simd::ifelse(freq <= 0, -freq, freq);
        freq = simd::ifelse(freq >= Flimit, Flimit, freq);
        Q = simd::ifelse(Q <= 0.5, 0.5, Q);

        T dfreq = freq*Ts;
        T w = T(2*M_PI)*dfreq;
        T q = T(0.5)/Q;

        T R = simd::exp(-q*w);
        T phi = simd::sqrt(1-q*q)*w;
        alpha = R*simd::cos(phi);
        beta = R*simd::sin(phi);

        T a[3] = {T(1),T(0),T(0)};
        a[1] = T(-2)*alpha;
        a[2] = alpha*alpha + beta*beta;

        T f02 = dfreq*dfreq;
        T r1 = (a[0] - a[1] + a[2])/simd::sqrt((1-f02)*(1-f02) + f02/(Q*Q));

        b[0] = T(0.25)*r1;
        b[1] = T(-2)*b[0];
        b[2] = b[0];
    }

    T process(T signal)
    {        
        x[2] = x[1];
        x[1] = x[0];
        re1[1] = re1[0];
        im1[1] = im1[0];
        re2[1] = re2[0];
        im2[1] = im2[0];

        x[0] = signal;
        signal = b[0]*x[0] + b[1]*x[1] + b[2]*x[2];

        re1[0] = signal + alpha*re1[1] - beta*im1[1];
        im1[0] = alpha*im1[1] + beta*re1[1];
        re2[0] = re1[0] + alpha*re2[1];

        return re2[0];
    }
};

}