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
    void setPhase(T phi, T mask)
    {
        phase = rack::simd::ifelse(mask, phi, phase);
    }

    void rewindPhase(T subsample_time, T mask)
    {
        phase = rack::simd::ifelse(mask, phase_z + d_freq*(1.f-subsample_time), phase);
    }

    void setDiscreteFrequency(T df)
    {
        d_freq = rack::simd::ifelse((df > 0.5f), 0.5f, df);
    }
    
    T getDiscreteFrequency(void)
    {
        return d_freq;
    }
    
    void timeStep(void)
    {
        phase_z = phase;
        phase = phase + d_freq;
        phase -= rack::simd::ifelse((phase >= 1.f), 1.f, 0.f);
    }

    T overturned(void)
    {
        return (phase_z > phase);
    }

    T getOverturnDelay(void)
    {
        return (phase/d_freq);
    }

    T getSawSample(void)
    {
        return (2.f*phase - 1.f);
    }

    T getSineSample(void)
    {
        return rack::simd::sin(2.f*M_PI*phase);
    }
};

#define N 32
#define reso 64

template <typename T>
struct DelayBuffer {
private:
    T buffer[N+1] = {};
    unsigned const length = N+1;
    unsigned index = 0;

public:
    T timeStep(T in_sample)
    {
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

template <typename T>
struct CorrectionBuffer {
private:
    T buffer[2*N] = {};
    unsigned const length = 2*N;
    unsigned index = 0;

    T getBlepResidual(T d_int, T d_frac, T mask, unsigned i);
    T getBlampResidual(T d_int, T d_frac, T mask, unsigned i);

public:
    void addDiscontinuity(T subsample_delay, T size, T mask, Discontinuity order)
    {
        mask = rack::simd::ifelse(((subsample_delay < 0.f) | (subsample_delay >= 1.f)), 0.f, mask);
        T d = length*subsample_delay;
        T d_int = std::floor(d);
        T d_frac = d - d_int;
        for(unsigned i = 0; i < length; i++){
            T residual = 0.f;
            switch(order){
            case Discontinuity::FIRST_ORDER:
                residual = getBlepResidual(d_int, d_frac, mask, i);
                break;
            case Discontinuity::SECOND_ORDER:
                residual = getBlampResidual(d_int, d_frac, mask, i);
                break;
            default:
                break;
            };
            
            unsigned p = index + i;
            if(p >= length) p -= length;
            buffer[p] += rack::simd::ifelse(mask, size*residual, 0.f);
        }
    }

    T timeStep(void)
    {
        T ret = buffer[index];
        buffer[index] = 0.f;
        index++;
        if(index >= length) index = 0;
        return ret;
    }
};

struct BandlimitedSaw {
private:
    Phasor<float> phasor;
	DelayBuffer<float> delay;
	CorrectionBuffer<float> correction;
    float tau = 1.f/48000.f;
    float freq = 0.f;

public:
    void setSampleTime(float sample_time)
    {
        tau = sample_time;
    }

    void setFrequency(float frequency)
    {
        freq = frequency;
        phasor.setDiscreteFrequency(tau*frequency);
    }

    void reset(void)
    {
        phasor.setPhase(0.f, 1);
        for(unsigned i = 0; i < N; i++){
            (void)process();
        }
    }

    bool synced = false;
    float sync_delay = 0.f;
    bool overturned = false;
    float overturn_delay = 0.f;

    void sync(float subsample_delay)
    {
        if(0.5f > phasor.getDiscreteFrequency()){
            sync_delay = subsample_delay;
            synced = true;
        }
    }

    float process(void)
    {
		float ret = 5.f*(delay.timeStep(phasor.getSawSample()) + correction.timeStep());
		phasor.timeStep();
        if(synced && phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
            if(overturn_delay > sync_delay){
                correction.addDiscontinuity(overturn_delay, -2.f, 1, Discontinuity::FIRST_ORDER);
            }
            phasor.rewindPhase(sync_delay, 1);
            if(overturn_delay <= sync_delay){
                correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), 1, Discontinuity::FIRST_ORDER);
            }
            phasor.setPhase(tau*freq*sync_delay, 1);
            synced = false;
        }
        else if(synced){
            phasor.rewindPhase(sync_delay, 1);
            correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), 1, Discontinuity::FIRST_ORDER);
            phasor.setPhase(tau*freq*sync_delay, 1);
            synced = false;
        }
        else if(phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
			correction.addDiscontinuity(overturn_delay, -2.f, 1, Discontinuity::FIRST_ORDER);
		}
        else{
            overturned = false;
        }
        return ret;
    }

    float getAliasedSawSample(void)
    {
        return 5.f*phasor.getSawSample();
    }
};
}
