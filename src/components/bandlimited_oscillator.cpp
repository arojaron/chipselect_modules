#include "bandlimited_oscillator.hpp"

float const cs::blep_table[reso+1][2*N] = {
    #include "blep_table.dat"
};
float const cs::blamp_table[reso+1][2*N] = {
    #include "blamp_table.dat"
};
