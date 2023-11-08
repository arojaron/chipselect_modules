#pragma once

#include "rack.hpp"

namespace cs{

template <typename T>
struct SimpleSvf {

    T Ts;
    T Flimit;
    T g;
    T R;

    T m1 = 0.f;
    T m2 = 0.f;

    T hp = 0.f;
    T bp = 0.f;
    T lp = 0.f;

    T signal = 0.f;
    
    SimpleSvf(T FS) : Ts(T(1.f/FS)), Flimit(T(0.45f*FS)), g(simd::tan(T(M_PI*100.f)*Ts)), R(T(1.f)) {}

    void setParams(T freq, T Q) {
        Q = simd::ifelse(Q < 0.5f, 0.5f, Q);
        freq = simd::ifelse(freq <= 0.f, 0.f, freq);
        freq = simd::ifelse(freq >= Flimit, Flimit, freq);

        R = T(1.f)/Q;
        g = simd::tan(T(M_PI)*freq*Ts);
    }

    T process(T in) {
        signal = in;
        hp = (in-(g + T(2.f)*R)*m1 - m2)/(g*g + T(2.f)*g*R + T(1.f));
        bp = g*hp + m1;
        lp = g*bp + m2;
        m1 = g*hp + bp;
        m2 = g*bp + lp;

        return lp;
    }

    T getLowPass(void) {
        return lp;
    }

    T getBandPass(void) {
        return bp;
    }

    T getHighPass(void) {
        return hp;
    }

    T getAllPass(void) {
        return T(2.f)*(hp+lp)-signal;
    }
};

template <typename T, unsigned N>
struct SeriesAllpass {

    T Ts;
    T Flimit;
    T g;
    T R;

    T m1[N] = {0.f};
    T m2[N] = {0.f};

    T hp[N] = {0.f};
    T bp[N] = {0.f};
    T lp[N] = {0.f};

    T signal[N] = {0.f};
    
    SeriesAllpass(T FS) : Ts(T(1.f/FS)), Flimit(T(0.45f*FS)), g(simd::tan(T(M_PI*100.f)*Ts)), R(T(1.f)) {}

    void setParams(T freq, T Q) {
        Q = simd::ifelse(Q < 0.5f, 0.5f, Q);
        freq = simd::ifelse(freq <= 0.f, 0.f, freq);
        freq = simd::ifelse(freq >= Flimit, Flimit, freq);

        R = T(1.f)/Q;
        g = simd::tan(T(M_PI)*freq*Ts);
    }

    void process(T in, unsigned depth) {
        if (depth > N) depth = N;
        for(unsigned i = 0; i < depth; i++){
            signal[i] = in;
            hp[i] = (in-(g + T(2.f)*R)*m1[i] - m2[i])/(g*g + T(2.f)*g*R + T(1.f));
            bp[i] = g*hp[i] + m1[i];
            lp[i] = g*bp[i] + m2[i];
            m1[i] = g*hp[i] + bp[i];
            m2[i] = g*bp[i] + lp[i];
            in = T(2.f)*(hp[i]+lp[i])-signal[i];
        }
    }

    T getAllPass(unsigned depth) {
        if (depth > N || depth == 0) return T(0.f);
        return T(2.f)*(hp[depth-1]+lp[depth-1])-signal[depth-1];
    }
};
}