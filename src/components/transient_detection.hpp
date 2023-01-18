#pragma once

#include "rack.hpp"
#include "one_pole.hpp"

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

struct TransientDetector{
    float Ts;
    OnePole<float> filter_post;
    dsp::ExponentialSlewLimiter follower;
    dsp::ExponentialSlewLimiter lagger;

    float follower_rise_tau = 0.0003;
    float lagger_rise_tau = 0.003;
    float fall_tau = 0.1;

    float following = 0;
    float lagging = 0;

    TransientDetector(float FS) 
    : Ts(1.f/FS),
      filter_post(OnePole<float>(FS))
    {
        filter_post.setFrequency(100.f);
        follower.setRiseFallTau(follower_rise_tau, fall_tau);
        lagger.setRiseFallTau(lagger_rise_tau, fall_tau);
    }

    void setRiseTaus(float follower_rise, float lagger_rise)
    {
        follower_rise_tau = follower_rise;
        lagger_rise_tau = lagger_rise;
        follower.setRiseFallTau(follower_rise_tau, fall_tau);
        lagger.setRiseFallTau(lagger_rise_tau, fall_tau);
    }

    float process(float signal)
    {
        signal = std::abs(signal);
        following = follower.process(Ts, signal);
        lagging = lagger.process(Ts, signal);
        return filter_post.process(sigmoid(following - lagging));
    }
};

}