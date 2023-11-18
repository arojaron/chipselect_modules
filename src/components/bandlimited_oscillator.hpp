#pragma once

#include "rack.hpp"
#include <vector>

namespace cs {

struct Phasor {
private:
    float phase = 0.f;
    float phase_z = 0.f;
    float d_freq = 0.f;

public:
    void setPhase(float phi)
    {
        phase = phi;
    }

    void rewindPhase(float subsample_time)
    {
        phase = phase_z + d_freq*(1.f-subsample_time);
    }

    void setDiscreteFrequency(float df)
    {
        d_freq = (df > 0.5f) ? 0.5f : df;
    }
    
    float getDiscreteFrequency(void)
    {
        return d_freq;
    }
    
    void timeStep(void)
    {
        phase_z = phase;
        phase = phase + d_freq;
        if(phase >= 1.f) phase -= 1.f;
    }

    bool overturned(void)
    {
        return (phase_z > phase);
    }

    float getOverturnDelay(void)
    {
        return (phase/d_freq);
    }

    float getSawSample(void)
    {
        return (2.f*phase - 1.f);
    }

    float getSineSample(void)
    {
        return std::sin(2.f*M_PI*phase);
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
    float timeStep(float in_sample)
    {
        buffer[index] = in_sample;
        index++;
        if(index >= length) index = 0;
        return buffer[index];
    }
};

struct CorrectionBuffer {
private:
    float buffer[2*N] = {};
    unsigned const length = 2*N;
    unsigned index = 0;
    static float const blep_table[reso+1][2*N];
    static float const blamp_table[reso+1][2*N];

public:
    enum Discontinuity {
        FIRST_ORDER = 0,
        SECOND_ORDER
    };
    void addDiscontinuity(float subsample_delay, float size, Discontinuity order)
    {
        if(subsample_delay < 0.f || subsample_delay >= 1.f) return;
        float d = length*subsample_delay;
        unsigned d_int = std::floor(d);
        float d_frac = d - d_int;
        for(unsigned i = 0; i < length; i++){
            float residual = 0.f;
            switch(order){
            case Discontinuity::FIRST_ORDER:
                residual = (1.f-d_frac)*blep_table[d_int][i] + d_frac*blep_table[d_int+1][i];
                break;
            case Discontinuity::SECOND_ORDER:
                residual = (1.f-d_frac)*blamp_table[d_int][i] + d_frac*blamp_table[d_int+1][i];
                break;
            default:
                break;
            };
            
            unsigned p = index + i;
            if(p >= length) p -= length;
            buffer[p] += size*residual;
        }
    }

    float timeStep(void)
    {
        float ret = buffer[index];
        buffer[index] = 0.f;
        index++;
        if(index >= length) index = 0;
        return ret;
    }
};

struct BandlimitedSaw {
private:
    Phasor phasor;
	DelayBuffer delay;
	CorrectionBuffer correction;
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
        phasor.setPhase(0.f);
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
                correction.addDiscontinuity(overturn_delay, -2.f, CorrectionBuffer::Discontinuity::FIRST_ORDER);
            }
            phasor.rewindPhase(sync_delay);
            if(overturn_delay <= sync_delay){
                correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), CorrectionBuffer::Discontinuity::FIRST_ORDER);
            }
            phasor.setPhase(tau*freq*sync_delay);
            synced = false;
        }
        else if(synced){
            phasor.rewindPhase(sync_delay);
            correction.addDiscontinuity(sync_delay, (-1.f - phasor.getSawSample()), CorrectionBuffer::Discontinuity::FIRST_ORDER);
            phasor.setPhase(tau*freq*sync_delay);
            synced = false;
        }
        else if(phasor.overturned()){
            overturned = true;
            overturn_delay = phasor.getOverturnDelay();
			correction.addDiscontinuity(overturn_delay, -2.f, CorrectionBuffer::Discontinuity::FIRST_ORDER);
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
