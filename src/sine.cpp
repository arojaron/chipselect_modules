#include "plugin.hpp"

#include "components/bandlimited_oscillator.hpp"

using namespace simd;

struct Sine : Module {
	enum ParamId {
		RATIO_NUM_PARAM,
		RATIO_DEN_PARAM,
		PHASE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VPOCT_INPUT,
		RESET_INPUT,
		PHASE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::BooleanTrigger reset_trigger[4];
	cs::Phasor<float_4> osc;

	Sine() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RATIO_NUM_PARAM, 1.f, 20.f, 1.f, "Frequency ratio numerator");
		getParamQuantity(RATIO_NUM_PARAM)->snapEnabled = true;
		configParam(RATIO_DEN_PARAM, 1.f, 20.f, 1.f, "Frequency ratio denominator");
		getParamQuantity(RATIO_DEN_PARAM)->snapEnabled = true;
		configParam(PHASE_PARAM, -1.f, 1.f, 0.f, "Phase modulation/offset");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(RESET_INPUT, "Reset");
		configInput(PHASE_INPUT, "Phase modulation");
		configOutput(SIGNAL_OUTPUT, "Sine");
	}

	void process(const ProcessArgs& args) override {
		unsigned num_channels = std::max<unsigned>(inputs[VPOCT_INPUT].getChannels(), inputs[PHASE_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[RESET_INPUT].getChannels());
		num_channels = std::min<unsigned>(num_channels, 4);
		outputs[SIGNAL_OUTPUT].setChannels(num_channels);

		float reset[4];
		for(unsigned i = 0; i < 4; i++){
			reset[i] = reset_trigger[i].process(inputs[RESET_INPUT].getVoltage(i)) ? 0xFFFFFFFF : 0x00000000;
		}
		float_4 reset_4 = float_4(reset[0], reset[1], reset[2], reset[3]);
		osc.setPhase(0.f, reset_4);

		float_4 pitch = inputs[VPOCT_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch);
		freq *= params[RATIO_NUM_PARAM].getValue();
		freq /= params[RATIO_DEN_PARAM].getValue();

		osc.setDiscreteFrequency(freq*args.sampleTime);

		osc.timeStep();

		float_4 phase_mod;
		if(inputs[PHASE_INPUT].isConnected()){
			phase_mod = inputs[PHASE_INPUT].getPolyVoltageSimd<float_4>(0) * dsp::cubic(params[PHASE_PARAM].getValue());
		}
		else{
			phase_mod = params[PHASE_PARAM].getValue();
		}

		outputs[SIGNAL_OUTPUT].setVoltageSimd(5.f*osc.getSineSample(phase_mod), 0);
	}
};


struct SineWidget : ModuleWidget {
	SineWidget(Sine* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Sine.svg")));

		addParam(createParamCentered<VCVSlider>(mm2px(Vec(10.16, 29.21)), module, Sine::RATIO_NUM_PARAM));
		addParam(createParamCentered<VCVSlider>(mm2px(Vec(20.32, 29.21)), module, Sine::RATIO_DEN_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.32, 91.44)), module, Sine::PHASE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 55.88)), module, Sine::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 73.66)), module, Sine::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 91.44)), module, Sine::PHASE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 107.95)), module, Sine::SIGNAL_OUTPUT));
	}
};


Model* modelSine = createModel<Sine, SineWidget>("Sine");