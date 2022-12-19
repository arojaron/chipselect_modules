#pragma once
#include "delay_granular.hpp"

typedef float StageLengths[4];

struct DelayStage4{
private:
    GrainClock clock;
    Delay2H line0;
    Delay2H line1;
    Delay2H line2;
    Delay2H line3;
    float delay_scale = 0.0;
    float delay_scale_current = 0.0;
	float delay_scale_previous = 0.0;
    float grain_sharpness_current = 2.0;  // [1.0; inf)
    float grain_sharpness = 2.0;  // [1.0; inf)

public:
    DelayStage4(StageLengths lengths, float FS)
    : clock(GrainClock(0.01*FS)), 
      line0(Delay2H(lengths[0]*FS)), 
      line1(Delay2H(lengths[1]*FS)), 
      line2(Delay2H(lengths[2]*FS)), 
      line3(Delay2H(lengths[3]*FS)) {}

    DelayStage4& operator=(DelayStage4 const& other)
    {
        clock = other.clock;
        line0 = other.line0;
        line1 = other.line1;
        line2 = other.line2;
        line3 = other.line3;
        return *this;
    }
      
    void setScale(float scale)
    {
        delay_scale = scale;
    }

    void setSharpness(float sharpness)
    {
        grain_sharpness = sharpness;
    }

    simd::float_4 process(simd::float_4 in)
    {
        if(clock.process()){
			delay_scale_previous = delay_scale_current;
			delay_scale_current = delay_scale;
            grain_sharpness_current = grain_sharpness;
		}
		float index = clock.getIndex();

		float A;
		float B;
        float ret[4];
		line0.step(in[0], delay_scale_current, delay_scale_previous, &A, &B);
		ret[0] = xfade(A, B, grain_sharpness_current*index);
        line1.step(in[1], delay_scale_current, delay_scale_previous, &A, &B);
		ret[1] = xfade(A, B, grain_sharpness_current*index);
        line2.step(in[2], delay_scale_current, delay_scale_previous, &A, &B);
		ret[2] = xfade(A, B, grain_sharpness_current*index);
        line3.step(in[3], delay_scale_current, delay_scale_previous, &A, &B);
		ret[3] = xfade(A, B, grain_sharpness_current*index);

        return simd::float_4::load(ret);
    }
};