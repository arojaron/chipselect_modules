#pragma once

#include "delay_stage_4.hpp"
#include "matrix_mixer_4.hpp"

struct DiffusionStage4{
private:
	float FS;
    DelayStage4 delay_stage_1;
    DelayStage4 delay_stage_2;
    DelayStage4 delay_stage_3;
    DelayStage4 delay_stage_4;
    MatrixMixer4 mixer_1;
    MatrixMixer4 mixer_2;
    MatrixMixer4 mixer_3;
    MatrixMixer4 mixer_4;

public:
    DiffusionStage4(simd::float_4 lengths[4], simd::float_4 mixer_normals[4], float FS)
    : FS(FS),
      delay_stage_1(DelayStage4(lengths[0], FS)),
      delay_stage_2(DelayStage4(lengths[1], FS)),
      delay_stage_3(DelayStage4(lengths[2], FS)),
      delay_stage_4(DelayStage4(lengths[3], FS)),
      mixer_1(MatrixMixer4(mixer_normals[0])),
      mixer_2(MatrixMixer4(mixer_normals[1])),
      mixer_3(MatrixMixer4(mixer_normals[2])),
      mixer_4(MatrixMixer4(mixer_normals[3])) {}

    DiffusionStage4& operator=(DiffusionStage4 const& other)
    {
        FS = other.FS;
        delay_stage_1 = other.delay_stage_1;
        delay_stage_2 = other.delay_stage_2;
        delay_stage_3 = other.delay_stage_3;
        delay_stage_4 = other.delay_stage_4;
        mixer_1 = other.mixer_1;
        mixer_2 = other.mixer_2;
        mixer_3 = other.mixer_3;
        mixer_4 = other.mixer_4;
        return *this;
    }
    
    void setScale(float scale)
    {
        delay_stage_1.setScale(scale);
        delay_stage_2.setScale(scale);
        delay_stage_3.setScale(scale);
        delay_stage_4.setScale(scale);
    }

    simd::float_4 process(simd::float_4 in)
    {
        in = mixer_1.process(delay_stage_1.process(in));
        in = mixer_2.process(delay_stage_2.process(in));
        in = mixer_3.process(delay_stage_3.process(in));
        in = mixer_4.process(delay_stage_4.process(in));
        return in;
    }
};
