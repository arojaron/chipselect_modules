#pragma once

#include "rack.hpp"

namespace cs{

inline float sigmoid(float signal)
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
        slew_limiter.setRiseFall(1, 0.001);
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

struct TransientDetector{
    float Ts;
    dsp::ExponentialSlewLimiter follower;
    dsp::ExponentialSlewLimiter lagger;

    float following = 0;
    float lagging = 0;

    TransientDetector(float FS) : Ts(1.f/FS)
    {
        follower.setRiseFallTau(0.001, 0.1);
        lagger.setRiseFallTau(0.01, 0.1);
    }

    float process(float signal)
    {
        signal = 0.01*signal*signal;
        following = follower.process(Ts, signal);
        lagging = lagger.process(Ts, signal);
        return sigmoid(following - lagging);
    }
};

}