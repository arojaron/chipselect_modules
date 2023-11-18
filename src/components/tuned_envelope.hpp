#pragma once

namespace cs{

template <typename T = float>
struct TriggerProcessor {
	T state = 0.f;

	T process(T in) {
		T triggered = simd::ifelse(state == 0.f, simd::ifelse(in > 0.f, 1.f, 0.f), 0.f);
		state = in;
		return triggered;
	}
}; 

template <typename T = float>
struct TunedDecayEnvelope {
	T out = 0.f;
	T fall = 0.f;

	void setFrequency(T freq) {
		this->fall = simd::abs(10.f * freq);
	}
	T process(T deltaTime, T in) {
		out = simd::fmax(in, out - fall * deltaTime);
		return out;
	}
};

}
