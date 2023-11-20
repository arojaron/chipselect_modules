#include "bandlimited_oscillator.hpp"

float const cs::blep_table[reso+1][2*N] = {
    #include "blep_table.dat"
};
float const cs::blamp_table[reso+1][2*N] = {
    #include "blamp_table.dat"
};

template<>
float cs::CorrectionBuffer<float>::getBlepResidual(float d_int, float d_frac, float mask, unsigned i) {
    float ret = 0.f;
    if(mask){
        ret = (1.f-d_frac)*blep_table[(unsigned)d_int][i] + d_frac*blep_table[(unsigned)d_int+1][i];
    }
    return ret;
}
template<>
float cs::CorrectionBuffer<float>::getBlampResidual(float d_int, float d_frac, float mask, unsigned i) {
    float ret = 0.f;
    if(mask){
        ret = (1.f-d_frac)*blamp_table[(unsigned)d_int][i] + d_frac*blamp_table[(unsigned)d_int+1][i];
    }
    return ret;
}
