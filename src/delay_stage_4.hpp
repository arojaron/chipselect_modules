#pragma once
#include "delay_granular.hpp"

struct DelayStage4{
private:
    GrainClock clock;
    Delay2H lines[4];
    simd::float_4 delay_scale = simd::float_4::zero();
    simd::float_4 scale_current = simd::float_4::zero();
	simd::float_4 scale_previous = simd::float_4::zero();
    float grain_sharpness = 4.0;    // [1.0; inf)
    float grain_period = 0.01;      // s

public:
    DelayStage4(simd::float_4 lengths, float FS)
    : clock(GrainClock(grain_period*FS)), 
      lines{
        Delay2H(lengths[0]*FS), 
        Delay2H(lengths[1]*FS), 
        Delay2H(lengths[2]*FS), 
        Delay2H(lengths[3]*FS)} {}

    DelayStage4& operator=(DelayStage4 const& other)
    {
        clock = other.clock;
        lines[0] = other.lines[0];
        lines[1] = other.lines[1];
        lines[2] = other.lines[2];
        lines[3] = other.lines[3];
        return *this;
    }
      
    void setScale(float scale)
    {
        delay_scale = simd::float_4(scale);
    }
    
    void setScale(simd::float_4 scale)
    {
        delay_scale = scale;
    }

    simd::float_4 process(simd::float_4 in)
    {
        if(clock.process()){
			scale_previous = scale_current;
			scale_current = delay_scale;
		}
		float index = clock.getIndex();

		float A;
		float B;
        simd::float_4 ret;
        for(unsigned i = 0; i < 4; i++){
            lines[i].step(in[i], scale_current[i], scale_previous[i], &A, &B);
		    ret[i] = xfade(A, B, grain_sharpness*index);
        }

        return ret;
    }
};