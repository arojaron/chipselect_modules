#include "plugin.hpp"
#include "components/matched_shelving.hpp"


struct TwoShelf : Module {
	enum ParamId {
		HIGH_PARAM,
		FREQ_PARAM,
		LOW_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	cs::TwoShelves<float> filter;

	TwoShelf() 
	: filter(cs::TwoShelves<float>(48000.f)){
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(HIGH_PARAM, -30.f, 30.f, 0.f, "");
		configParam(FREQ_PARAM, 0.f, 1.f, 0.1f, "");
		configParam(LOW_PARAM, -30.f, 30.f, 0.f, "");
		configInput(SIGNAL_INPUT, "");
		configOutput(SIGNAL_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override 
	{
		float G0 = dsp::dbToAmplitude(params[LOW_PARAM].getValue());
		float G1 = dsp::dbToAmplitude(params[HIGH_PARAM].getValue());
		float freq = args.sampleRate * 0.5f * params[FREQ_PARAM].getValue();

		filter.setParams(freq, G0, G1);
		outputs[SIGNAL_OUTPUT].setVoltage(filter.process(inputs[SIGNAL_INPUT].getVoltage()));
	}
};


struct TwoShelfWidget : ModuleWidget {
	TwoShelfWidget(TwoShelf* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/two_shelf.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 37.771)), module, TwoShelf::HIGH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 59.499)), module, TwoShelf::FREQ_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 81.227)), module, TwoShelf::LOW_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.723, 116.693)), module, TwoShelf::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.757, 116.693)), module, TwoShelf::SIGNAL_OUTPUT));
	}
};


Model* modelTwoShelf = createModel<TwoShelf, TwoShelfWidget>("TwoShelf");