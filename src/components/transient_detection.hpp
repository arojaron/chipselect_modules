#pragma once

#include "rack.hpp"

float sigmoid(float signal)
{
    if(signal > 1.0){
        return 1.0;
    }
    if(signal < 0.0){
        return 0.0;
    }

    return signal*signal*(3.0 - 2.0*signal);
}

struct Ducking{
    float scaling = 0.0;
    dsp::SlewLimiter slew_limiter;

    Ducking()
    {
        slew_limiter.setRiseFall(10, 0.0001);
    }
    void setScaling(float scale)
    {
        scaling = scale;
    }
    float process(float signal)
    {
        return sigmoid(slew_limiter.process(scaling*signal*signal));
    }
};