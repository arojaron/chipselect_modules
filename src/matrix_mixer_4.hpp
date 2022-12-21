#pragma once
#include "plugin.hpp"

typedef float const Mixer4Coefficients[16];
typedef simd::float_4 const NormalVector4;

struct MatrixMixer4{
private:
    simd::float_4 rows[4];

    float sumFloat4(simd::float_4 v)
    {
        return v[0] + v[1] + v[2] + v[3];
    }

public:
    MatrixMixer4(Mixer4Coefficients coefs)
    {
        rows[0] = simd::float_4(coefs[0], coefs[1], coefs[2], coefs[3]);
        rows[1] = simd::float_4(coefs[4], coefs[5], coefs[6], coefs[7]);
        rows[2] = simd::float_4(coefs[8], coefs[9], coefs[10], coefs[11]);
        rows[3] = simd::float_4(coefs[12], coefs[13], coefs[14], coefs[15]);
    }
    MatrixMixer4(NormalVector4 normal)
    {
        simd::float_4 n0 = simd::float_4(normal[0]);
        simd::float_4 n1 = simd::float_4(normal[1]);
        simd::float_4 n2 = simd::float_4(normal[2]);
        simd::float_4 n3 = simd::float_4(normal[3]);
        simd::float_4 n = normal;
        simd::float_4 I0 = simd::float_4(1, 0, 0, 0);
        simd::float_4 I1 = simd::float_4(0, 1, 0, 0);
        simd::float_4 I2 = simd::float_4(0, 0, 1, 0);
        simd::float_4 I3 = simd::float_4(0, 0, 0, 1);
        simd::float_4 scaler = simd::float_4(2.0/sumFloat4(n*n));
        rows[0] = I0 - scaler*n0*n;
        rows[1] = I1 - scaler*n1*n;
        rows[2] = I2 - scaler*n2*n;
        rows[3] = I3 - scaler*n3*n;
    }

    simd::float_4 process(simd::float_4 v)
    {
        float c1 = sumFloat4(v*rows[0]);
        float c2 = sumFloat4(v*rows[1]);
        float c3 = sumFloat4(v*rows[2]);
        float c4 = sumFloat4(v*rows[3]);
        return simd::float_4(c1, c2, c3, c4);
    }
};