#include "plugin.hpp"

#include "components/matched_shelving.hpp"

struct TNT : Module {
	enum ParamId {
		PRE_TONE_PARAM,
		PRE_FREQ_PARAM,
		POST_FREQ_PARAM,
		DRIVE_PARAM,
		POST_TONE_PARAM,
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

	cs::Tilting<float> pre_tone;
	cs::Tilting<float> post_tone;

	TNT() 
	: pre_tone(cs::Tilting<float>(48000.f)), post_tone(cs::Tilting<float>(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PRE_TONE_PARAM, 30.f, -30.f, 0.f, "Pre shelving tilt", "dB");
		configParam(PRE_FREQ_PARAM, 100.f, 1000.f, 100.f, "Pre shelving freq");
		configParam(POST_TONE_PARAM, 30.f, -30.f, 0.f, "Post shelving tilt", "dB");
		configParam(POST_FREQ_PARAM, 100.f, 1000.f, 100.f, "Post shelving freq");
		configParam(DRIVE_PARAM, -40.f, 40.f, 0.f, "Drive gain", "dB");
		configInput(SIGNAL_INPUT, "");
		configOutput(SIGNAL_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override
	{
		float pre_freq = params[PRE_FREQ_PARAM].getValue();
		float pre_gain = std::pow(10.f, params[PRE_TONE_PARAM].getValue()/20.f);
		float post_freq = params[POST_FREQ_PARAM].getValue();
		float post_gain = std::pow(10.f, params[POST_TONE_PARAM].getValue()/20.f);
		float drive = std::pow(10.f, params[DRIVE_PARAM].getValue()/20.f);

		pre_tone.setParams(pre_freq, pre_gain);
		post_tone.setParams(post_freq, post_gain);

		float signal = inputs[SIGNAL_INPUT].getVoltage();
		signal = pre_tone.process(signal);
		signal = 10.f*std::tanh(0.1f*drive*signal);
		signal = post_tone.process(signal);

		outputs[SIGNAL_OUTPUT].setVoltage(signal);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		pre_tone = cs::Tilting<float>(e.sampleRate);
		post_tone = cs::Tilting<float>(e.sampleRate);
	}
};


struct TNTWidget : ModuleWidget {
	TNTWidget(TNT* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/tnt.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 22.86)), module, TNT::PRE_TONE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.75, 26.67)), module, TNT::PRE_FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.604, 100.465)), module, TNT::POST_FREQ_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(27.94, 54.61)), module, TNT::DRIVE_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(21.59, 86.36)), module, TNT::POST_TONE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.007, 116.607)), module, TNT::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.519, 118.785)), module, TNT::SIGNAL_OUTPUT));
	}
};


Model* modelTNT = createModel<TNT, TNTWidget>("TNT");