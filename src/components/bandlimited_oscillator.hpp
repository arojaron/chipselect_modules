#pragma once

#include "rack.hpp"
#include <vector>

namespace cs {

template <typename T>
struct Phasor {
private:
    T phase = 0.f;
    T phase_z = 0.f;
    T d_freq = 0.f;

public:
    void setPhase(T phi, T mask) {
        phase = rack::simd::ifelse(mask, phi, phase);
    }

    void rewindPhase(T subsample_time, T mask) {
        phase = rack::simd::ifelse(mask, phase_z + d_freq*(1.f-subsample_time), phase);
    }

    void setDiscreteFrequency(T df) {
        d_freq = rack::simd::ifelse((df > 0.5f), 0.5f, df);
    }
    
    T getDiscreteFrequency(void) {
        return d_freq;
    }
    
    void timeStep(void) {
        phase_z = phase;
        phase = phase + d_freq;
        phase -= rack::simd::ifelse((phase >= 1.f), 1.f, 0.f);
    }

    T overturned(void) {
        return (phase_z > phase);
    }

    T getOverturnDelay(void) {
        return (phase/d_freq);
    }

    T halfOverturned(void) {
        return ((phase_z <= 0.5f) && (phase > 0.5));
    }

    T getHalfOverturnDelay(void) {
        return (phase - 0.5f)/d_freq;
    }

    T getSawSample(void) {
        return (2.f*phase - 1.f);
    }

    T getTriangleSample(void) {
        T ret = phase * 4.f;
        ret = rack::simd::ifelse(ret > 2.f, 4.f-ret, ret);
        ret -= T(1.f);
        return ret;
    }

    T getSineSample(T phase_offset) {
        return rack::simd::sin(2.f*M_PI*(phase+phase_offset));
    }
};

#define N 32
#define reso 64

struct DelayBuffer {
private:
    float buffer[N+1] = {};
    unsigned const length = N+1;
    unsigned index = 0;

public:
    float timeStep(float in_sample) {
        buffer[index] = in_sample;
        index++;
        if(index >= length) index = 0;
        return buffer[index];
    }
};

extern float const blep_table[reso+1][2*N];
extern float const blamp_table[reso+1][2*N];

enum Discontinuity {
    FIRST_ORDER = 0,
    SECOND_ORDER
};

struct CorrectionBuffer {
private:
    float buffer[2*N] = {};
    unsigned const length = 2*N;
    unsigned index = 0;

public:
    void addDiscontinuity(float subsample_delay, float size, Discontinuity order) {
        if(((subsample_delay < 0.f) || (subsample_delay >= 1.f))) return;
        float d = length*subsample_delay;
        unsigned d_int = rack::simd::floor(d);
        float d_frac = d - d_int;
        for(unsigned i = 0; i < length; i++){
            float residual = 0.f;
            switch(order){
            case Discontinuity::FIRST_ORDER:
                residual = (1.f-d_frac)*blep_table[(unsigned)d_int][i] + d_frac*blep_table[(unsigned)d_int+1][i];
                break;
            case Discontinuity::SECOND_ORDER:
                residual = (1.f-d_frac)*blamp_table[(unsigned)d_int][i] + d_frac*blamp_table[(unsigned)d_int+1][i];
                break;
            default:
                break;
            };
            
            unsigned p = index + i;
            if(p >= length) p -= length;
            buffer[p] += size*residual;
        }
    }

    float timeStep(void) {
        float ret = buffer[index];
        buffer[index] = 0.f;
        index++;
        if(index >= length) index = 0;
        return ret;
    }
};

struct Saw {
private:
    Phasor<float> phasor;
	DelayBuffer delay;
	CorrectionBuffer correction;
    float tau = 1.f/48000.f;
    float freq = 0.f;

public:
    void setSampleTime(float sample_time) {
        tau = sample_time;
    }

    void setFrequency(float frequency) {
        freq = frequency;
        phasor.setDiscreteFrequency(tau*frequency);
    }

    void reset(void) {
        phasor.setPhase(0.f, 1);
        for(unsigned i = 0; i < N; i++){
            (void)process();
        }
    }

    void sync(float subsample_delay) {
        if(0.5f > phasor.getDiscreteFrequency()){
            sync_delay = subsample_delay;
            synced = true;
        }
    }

    bool synced = 0.f;
    float sync_delay = 0.f;
    bool overturned = 0.f;
    float overturn_delay = 0.f;

    float process(void) {
        float ret = 5.f*(delay.timeStep(phasor.getSawSample()) + correction.timeStep());
        phasor.timeStep();

        if(synced && phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
            if(overturn_delay > sync_delay){
                correction.addDiscontinuity(overturn_delay, -2.f, Discontinuity::FIRST_ORDER);
            }
            phasor.rewindPhase(sync_delay, true);
            if(overturn_delay <= sync_delay){
                correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), Discontinuity::FIRST_ORDER);
            }
            phasor.setPhase(tau*freq*sync_delay, true);
            synced = false;
        }
        else if(synced){
            phasor.rewindPhase(sync_delay, true);
            correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), Discontinuity::FIRST_ORDER);
            phasor.setPhase(tau*freq*sync_delay, true);
            synced = false;
        }
        else if(phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
            correction.addDiscontinuity(overturn_delay, -2.f, Discontinuity::FIRST_ORDER);
        }
        else{
            overturned = false;
        }
        return ret;
    }

    float getAliasedSample(void) {
        return 5.f*phasor.getSawSample();
    }
};

struct Triangle {
private:
    Phasor<float> phasor;
	DelayBuffer delay;
	CorrectionBuffer correction;
    float tau = 1.f/48000.f;
    float freq = 0.f;

public:
    void setSampleTime(float sample_time) {
        tau = sample_time;
    }

    void setFrequency(float frequency) {
        freq = frequency;
        phasor.setDiscreteFrequency(tau*frequency);
    }

    void reset(void) {
        phasor.setPhase(0.f, 1);
        for(unsigned i = 0; i < N; i++){
            (void)process();
        }
    }

    void sync(float subsample_delay) {
        if(0.5f > phasor.getDiscreteFrequency()){
            sync_delay = subsample_delay;
            synced = true;
        }
    }

    bool synced = 0.f;
    float sync_delay = 0.f;
    bool overturned = 0.f;
    float overturn_delay = 0.f;

    float process(void) {
        float ret = 5.f*(delay.timeStep(phasor.getTriangleSample()) + correction.timeStep());
        phasor.timeStep();

        if(synced){
            phasor.rewindPhase(sync_delay, true);
            correction.addDiscontinuity(sync_delay, (-1.f - phasor.getTriangleSample()), Discontinuity::FIRST_ORDER);
            phasor.setPhase(tau*freq*sync_delay, true);
            synced = false;
        }

        if(phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
            correction.addDiscontinuity(overturn_delay, 8.f*phasor.getDiscreteFrequency(), Discontinuity::SECOND_ORDER);
        }
        else if(phasor.halfOverturned()){
            correction.addDiscontinuity(phasor.getHalfOverturnDelay(), -8.f*phasor.getDiscreteFrequency(), Discontinuity::SECOND_ORDER);
        }
        else{
            overturned = false;
        }

        return ret;
    }

    float getAliasedSample(void) {
        return 5.f*phasor.getTriangleSample();
    }
};
}
