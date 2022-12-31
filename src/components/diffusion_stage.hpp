#pragma once

#include "delay_stage.hpp"
#include "matrix_mixer.hpp"

struct DiffusionStage{
private:
	float FS;
    DelayStage4 delay_stage;
    MatrixMixer4 mixer;

public:
    DiffusionStage(simd::float_4 lengths, simd::float_4 mixer_normal, float FS)
    : FS(FS),
      delay_stage(DelayStage4(lengths, FS)),
      mixer(MatrixMixer4(mixer_normal)) {}

    DiffusionStage& operator=(DiffusionStage const& other)
    {
        FS = other.FS;
        delay_stage = other.delay_stage;
        mixer = other.mixer;
        return *this;
    }
    
    void setScale(float scale)
    {
        delay_stage.setScale(scale);
    }

    void setScale(simd::float_4 scale)
    {
        delay_stage.setScale(scale);
    }

    simd::float_4 process(simd::float_4 in)
    {
        return mixer.process(delay_stage.process(in));
    }
};
