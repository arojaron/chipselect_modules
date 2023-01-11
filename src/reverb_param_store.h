#pragma once

#include "plugin.hpp"

struct ReverbParameters{
    simd::float_4 lengths[5] = {};
	simd::float_4 normals[5] = {
        simd::float_4(1, 1, 1, 1),
        simd::float_4(1, 1, 1, 1),
        simd::float_4(1, 1, 1, 1),
        simd::float_4(1, 1, 1, 1),
        simd::float_4(1, 1, 1, 1)
    };
};

unsigned loadReverbParameters(ReverbParameters& params, unsigned model_index);
void storeReverbParameters(ReverbParameters& params);