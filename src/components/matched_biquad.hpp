/*
Attempt at implementing some of the algorithms outlined in the paper titled
"Matched second order digital filter" by Martin Vicanek
https://www.vicanek.de/articles/BiquadFits.pdf
*/

#pragma once

#include "rack.hpp"
#include "math.h"

struct MatchedLowPass {
    rack::dsp::IIRFilter<3, 3, float> biquad;

    float T;
    float FS;
    float dfreq = 0.0;
    float Q = 1.0;

    float a[3] = {1,0,0};
    float b[3] = {1,0,0};

    MatchedLowPass(float FS) : T(1/FS), FS(FS) {}

    void setParams(float freq, float res)
    {
        dfreq = freq*T;
        Q = res;
        float q = 1/(2*Q);
        float w0 = 2*M_PI*dfreq;
        a[1] = -2*std::exp(-q*w0)*std::cos(w0*std::sqrt(1-q*q));
        a[2] = std::exp(-2*q*w0);

        float f02 = dfreq*dfreq;
        float r0 = 1 + a[1] + a[2];
        float r1 = f02*(1 - a[1] + a[2])/std::sqrt((1-f02)*(1-f02) + f02/(Q*Q));

        b[0] = 0.5*(r0 + r1);
        b[1] = r0 - b[0];
        b[2] = 0;

        biquad.setCoefficients(&b[0], &a[1]);
    }

    float process(float signal)
    {
        return biquad.process(signal);
    }
};

struct MatchedComplexLowPass {
    float T;
    float FS;
    float b[3] = {};

    float alpha = 0;
    float beta = 0;

    float x[3] = {};
    float re1[2] = {};
    float im1[2] = {};
    float re2[2] = {};
    float im2[2] = {};

    MatchedComplexLowPass(float FS) : T(1/FS), FS(FS) {}

    void setParams(float freq, float Q)
    {
        if(freq < 10) freq = 10;
        if(freq > FS/2) freq = FS/2;
        if(Q < 0.5) Q = 0.5;
        if(Q > 20) Q = 20;

        float dfreq = freq*T;
        float w = 2*M_PI*dfreq;
        float q = 1/(2*Q);

        alpha = std::exp(-q*w)*std::cos(std::sqrt(1-q*q)*w);
        beta = std::exp(-q*w)*std::sin(std::sqrt(1-q*q)*w);

        float a[3] = {1,0,0};
        a[1] = -2*alpha;
        a[2] = alpha*alpha + beta*beta;

        float f02 = dfreq*dfreq;
        float r0 = a[0] + a[1] + a[2];
        float r1 = f02*(a[0] - a[1] + a[2])/std::sqrt((1-f02)*(1-f02) + f02/(Q*Q));

        b[0] = 0.5*(r0 + r1);
        b[1] = r0 - b[0];
        b[2] = 0;
    }

    float process(float signal)
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