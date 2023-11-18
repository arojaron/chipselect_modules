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
        d_freq = rack::simd::ifelse(df > 0.5f, 0.5f, df);
    }
    
    T getDiscreteFrequency(void) {
        return d_freq;
    }
    
    void timeStep(void) {
        phase_z = phase;
        phase = phase + d_freq;
        phase -= rack::simd::ifelse(phase >= 1.f, 1.f, 0.f);
    }

    T overturned(void) {
        return (phase_z > phase);
    }

    T getOverturnDelay(void) {
        return (phase/d_freq);
    }

    T getSawSample(void) {
        return (2.f*phase - 1.f);
    }

    T getSineSample(void) {
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
    T timeStep(T in_sample) {
        buffer[index] = in_sample;
        index++;
        if(index >= length) index = 0;
        return buffer[index];
    }
};

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
    static T const blep_table[reso+1][2*N];
    static T const blamp_table[reso+1][2*N];

    T getBlepResidual(unsigned i, T d_int, T d_frac, T mask);
    T getBlampResidual(unsigned i, T d_int, T d_frac, T mask);

public:

    void addDiscontinuity(T subsample_delay, T size, Discontinuity order, T mask) {
        if(subsample_delay < T(0.f) || subsample_delay >= T(1.f)) return;
        T d = length*subsample_delay;
        T d_int = rack::simd::floor(d);
        T d_frac = d - d_int;
        for(unsigned i = 0; i < length; i++){
            T residual = 0.f;
            switch(order){
            case Discontinuity::FIRST_ORDER:
                residual = getBlepResidual(i, d_int, d_frac, mask);
                break;
            case Discontinuity::SECOND_ORDER:
                residual = getBlampResidual(i, d_int, d_frac, mask);
                break;
            default:
                break;
            };
            
            unsigned p = index + i;
            if(p >= length) p -= length;
            buffer[p] += rack::simd::ifelse(mask, size*residual, 0.f);
        }
    }

    T timeStep(void) {
        T ret = buffer[index];
        buffer[index] = 0.f;
        index++;
        if(index >= length) index = 0;
        return ret;
    }
};

template <typename T>
struct BandlimitedSaw {
private:
    Phasor<T> phasor;
	DelayBuffer<T> delay;
	CorrectionBuffer<T> correction;
    float tau = 1.f/48000.f;
    T freq = 0.f;

public:
    void setSampleTime(float sample_time) {
        tau = sample_time;
    }

    void setFrequency(T frequency) {
        freq = frequency;
        phasor.setDiscreteFrequency(tau*frequency);
    }

    void reset(T mask) {
        phasor.setPhase(0.f, mask);
        for(unsigned i = 0; i < N; i++){
            (void)process();
        }
    }

    T synced = 0.f;
    T sync_delay = 0.f;
    T overturned = 0.f;
    T overturn_delay = 0.f;

    void sync(T subsample_delay, T mask) {
        if(0.5f > phasor.getDiscreteFrequency()){
            sync_delay = subsample_delay;
            synced = mask;
        }
    }

    T process(void) {
		T ret = 5.f*(delay.timeStep(phasor.getSawSample()) + correction.timeStep());
		phasor.timeStep();
        if(synced && phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
            if(overturn_delay > sync_delay){
                correction.addDiscontinuity(overturn_delay, -2.f, Discontinuity::FIRST_ORDER, 1);
            }
            phasor.rewindPhase(sync_delay, 1);
            if(overturn_delay <= sync_delay){
                correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), Discontinuity::FIRST_ORDER, 1);
            }
            phasor.setPhase(tau*freq*sync_delay, 1);
            synced = false;
        }
        else if(synced){
            phasor.rewindPhase(sync_delay, 1);
            correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), Discontinuity::FIRST_ORDER, 1);
            phasor.setPhase(tau*freq*sync_delay, 1);
            synced = false;
        }
        else if(phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
			correction.addDiscontinuity(overturn_delay, -2.f, Discontinuity::FIRST_ORDER, 1);
		}
        else{
            overturned = false;
        }
        return ret;
    }

    T getAliasedSawSample(void) {
        return 5.f*phasor.getSawSample();
    }
};
}
