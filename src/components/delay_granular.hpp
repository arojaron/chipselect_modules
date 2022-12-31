#pragma once

#include "rack.hpp"
#include <vector>

struct Delay2H{
private:
    std::vector<float> buffer;
    unsigned capacity;
    unsigned write_head = 0;

public:
    Delay2H(unsigned cap) 
    : capacity(cap)
    {
        buffer = std::vector<float>(capacity, 0.0);
    }

    void step(float in, float delay_1, float delay_2, float* out_1, float* out_2)
    {
        if(delay_1 < 0.0) delay_1 = 0.0;
        if(delay_1 > 1.0) delay_1 = 1.0;
        if(delay_2 < 0.0) delay_2 = 0.0;
        if(delay_2 > 1.0) delay_2 = 1.0;
        
        if(!capacity){
            *out_1 = in;
            *out_2 = in;
            return;
        }
        
        buffer[write_head] = in;
        write_head++;
        if(write_head >= capacity) write_head = 0;

        unsigned delay_1_samples = (unsigned)(delay_1*capacity);
        if(delay_1_samples){
            int out_head = write_head - delay_1_samples;
            if(out_head < 0) out_head += capacity;
            *out_1 = buffer[out_head];
        }
        else{
            *out_1 = in;
        }
        unsigned delay_2_samples = (unsigned)(delay_2*capacity);
        if(delay_2_samples){
            int out_head = write_head - delay_2_samples;
            if(out_head < 0) out_head += capacity;
            *out_2 = buffer[out_head];
        }
        else{
            *out_2 = in;
        }
    }
};

inline float xfade(float A, float B, float index)
{
    if(index >= 1.0){
        return A;
    }
    float A_coef = index*index*(3.0 - 2.0*index);   // mixing up
    float B_coef = 1.0 - A_coef;                    // mixing down
    return A*A_coef + B*B_coef;
}

struct GrainClock{
private:
    unsigned period;
    float frequency;
    unsigned clock = 0;

public:
    GrainClock(unsigned per) 
    : period(per), frequency(1.0/(float)per) {}

    float getIndex(void)
    {
        return (float)clock*frequency;
    }

    bool process(void)
    {
        clock++;
        if (clock >= period){
            clock = 0;
            return true;
        }
        return false;
    }
};
