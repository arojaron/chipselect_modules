#pragma once

namespace cs{

    inline float sigmoid(float signal)
    {
        if(signal > 1.0){
            return 1.0;
        }
        if(signal < 0.0){
            return 0.0;
        }

        return signal*signal*(3.0 - 2.0*signal);
    }

    inline float xfade(float A, float B, float index)
    {
        if(index >= 1.f){
            return A;
        }
        float A_coef = index*index*(3.f - 2.f*index);   // mixing up
        float B_coef = 1.f - A_coef;                    // mixing down
        return A*A_coef + B*B_coef;
    }
}