/*
Attempt at implementing some of the algorithms outlined in the paper titled
"Matched second order digital filter" by Martin Vicanek
https://www.vicanek.de/articles/BiquadFits.pdf
Also utilizing the "complex one-pole" method described in
"Fast Modulation of Filter Parameters" by Ammar Muqaddas
http://www.solostuff.net/wp-content/uploads/2019/05/Fast-Modulation-of-Filter-Parameters-v1.1.1.pdf
*/

#pragma once

#include "rack.hpp"
#include "math.h"

namespace chipselect{

struct LowPass {
    float T;
    float FSp2;

    float b[3] = {};
    float alpha = 0;
    float beta = 0;

    float x[3] = {};
    float re1[2] = {};
    float im1[2] = {};
    float re2[2] = {};
    float im2[2] = {};

    LowPass(float FS) : T(1/FS), FSp2(FS/2) {}

    void setParams(float freq, float Q)
    {
        if(freq < 1) freq = 1;
        if(freq > FSp2) freq = FSp2;
        if(Q < 0.5) Q = 0.5;
        //if(Q > 10e6) Q = 10e6;

        float dfreq = freq*T;
        float w = 2*M_PI*dfreq;
        float q = 1/(2*Q);

        float R = std::exp(-q*w);
        float phi = std::sqrt(1-q*q)*w;
        alpha = R*std::cos(phi);
        beta = R*std::sin(phi);

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

}