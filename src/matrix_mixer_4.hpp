#pragma once
#include "plugin.hpp"

typedef float MixerCoefficients[16];

struct MatrixMixer4{
private:
    simd::float_4 row1;
    simd::float_4 row2;
    simd::float_4 row3;
    simd::float_4 row4;

public:
    MatrixMixer4(MixerCoefficients coefs)
    {
        row1 = simd::float_4(coefs[0], coefs[1], coefs[2], coefs[3]);
        row2 = simd::float_4(coefs[4], coefs[5], coefs[6], coefs[7]);
        row3 = simd::float_4(coefs[8], coefs[9], coefs[10], coefs[11]);
        row4 = simd::float_4(coefs[12], coefs[13], coefs[14], coefs[15]);
    }

    simd::float_4 process(simd::float_4 v)
    {
        auto sumFloat4 = [](simd::float_4 v){
            return v[0] + v[1] + v[2] + v[3];
        };
        float c1 = sumFloat4(v*row1);
        float c2 = sumFloat4(v*row2);
        float c3 = sumFloat4(v*row3);
        float c4 = sumFloat4(v*row4);
        return simd::float_4(c1, c2, c3, c4);
    }
};