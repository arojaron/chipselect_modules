#pragma once

#include "rack.hpp"

namespace cs{

struct SimpleSvf {

    float Ts;
    float Flimit;
    float g;
    float R;

    float m1 = 0.f;
    float m2 = 0.f;
    
    SimpleSvf(float FS) : Ts(1.f/FS), Flimit(0.45f*FS), g(std::tan(M_PI*100.f*Ts)), R(1.f) {}

    void setParams(float freq, float Q) {
        Q = Q < 0.5f ? 0.5f : Q;

        freq = freq < 0.f ? -freq : freq;
        freq = freq > Flimit ? Flimit : freq;

        R = 1.f/Q;
        g = std::tan(M_PI*freq*Ts);
    }

    float process(float in) {
        float hp = (in-(g + 2.f*R)*m1 - m2)/(g*g + 2.f*g*R + 1.f);
        float bp = g*hp + m1;
        float lp = g*bp + m2;
        m1 = g*hp + bp;
        m2 = g*bp + lp;

        return lp;
    }
};
}