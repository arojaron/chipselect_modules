#include "bandlimited_oscillator.hpp"

using namespace rack::simd;

template<>
float const cs::CorrectionBuffer<float>::blep_table[reso+1][2*N] = {
    #include "blep_table.dat"
};
template<>
float const cs::CorrectionBuffer<float>::blamp_table[reso+1][2*N] = {
    #include "blamp_table.dat"
};
template<>
float cs::CorrectionBuffer<float>::getBlepResidual(unsigned i, float d_int, float d_frac, float mask) {
    return ifelse(mask, (1.f-d_frac)*blep_table[(unsigned)d_int][i] + d_frac*blep_table[(unsigned)d_int+1][i], 0.f);;
}
template<>
float cs::CorrectionBuffer<float>::getBlampResidual(unsigned i, float d_int, float d_frac, float mask) {
    return ifelse(mask, (1.f-d_frac)*blamp_table[(unsigned)d_int][i] + d_frac*blamp_table[(unsigned)d_int+1][i], 0.f);
}

template<>
float_4 const cs::CorrectionBuffer<float_4>::blep_table[reso+1][2*N] = {
    #include "blep_table.dat"
};
template<>
float_4 const cs::CorrectionBuffer<float_4>::blamp_table[reso+1][2*N] = {
    #include "blamp_table.dat"
};
template<>
float_4 cs::CorrectionBuffer<float_4>::getBlepResidual(unsigned i, float_4 d_int, float_4 d_frac, float_4 mask) {
    return 0;
}
template<>
float_4 cs::CorrectionBuffer<float_4>::getBlampResidual(unsigned i, float_4 d_int, float_4 d_frac, float_4 mask) {
    return 0;
}