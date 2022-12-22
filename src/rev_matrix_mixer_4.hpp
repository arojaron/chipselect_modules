#pragma once

#include "plugin.hpp"

struct MatrixMixer4{
private:
    simd::float_4 rows[4];

    float sumFloat4(simd::float_4 v)
    {
        return v[0] + v[1] + v[2] + v[3];
    }

public:
    MatrixMixer4(simd::float_4 const normal)
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
        simd::float_4 ret;
        ret[0] = sumFloat4(v*rows[0]);
        ret[1] = sumFloat4(v*rows[1]);
        ret[2] = sumFloat4(v*rows[2]);
        ret[3] = sumFloat4(v*rows[3]);
        return ret;
    }
};