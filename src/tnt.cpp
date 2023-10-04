#include "plugin.hpp"

#include "components/matched_shelving.hpp"
#include "components/transient_detection.hpp"

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
		RIGHT_INPUT,
		LEFT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		RIGHT_OUTPUT,
		LEFT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	cs::Tilting<float> pre_left_tone;
	cs::Tilting<float> post_left_tone;
	cs::Tilting<float> pre_right_tone;
	cs::Tilting<float> post_right_tone;

	TNT() 
	: pre_left_tone(cs::Tilting<float>(48000.f)), 
	  post_left_tone(cs::Tilting<float>(48000.f)),
	  pre_right_tone(cs::Tilting<float>(48000.f)),
	  post_right_tone(cs::Tilting<float>(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PRE_TONE_PARAM, 30.f, -30.f, 0.f, "Pre shelving tilt", "dB");
		configParam(PRE_FREQ_PARAM, 10.f, 1000.f, 100.f, "Pre shelving freq");
		configParam(POST_TONE_PARAM, 30.f, -30.f, 0.f, "Post shelving tilt", "dB");
		configParam(POST_FREQ_PARAM, 10.f, 1000.f, 100.f, "Post shelving freq");
		configParam(DRIVE_PARAM, -40.f, 40.f, 0.f, "Drive gain", "dB");
		configInput(RIGHT_INPUT, "");
		configInput(LEFT_INPUT, "");
		configOutput(RIGHT_OUTPUT, "");
		configOutput(LEFT_OUTPUT, "");

		configBypass(LEFT_INPUT, LEFT_OUTPUT);
		configBypass(RIGHT_INPUT, RIGHT_OUTPUT);
	}

	void process(const ProcessArgs& args) override
	{
		float pre_freq = params[PRE_FREQ_PARAM].getValue();
		float pre_gain = std::pow(10.f, params[PRE_TONE_PARAM].getValue()/20.f);
		float post_freq = params[POST_FREQ_PARAM].getValue();
		float post_gain = std::pow(10.f, params[POST_TONE_PARAM].getValue()/20.f);
		float drive = std::pow(10.f, params[DRIVE_PARAM].getValue()/20.f);

		pre_left_tone.setParams(pre_freq, pre_gain);
		pre_right_tone.setParams(pre_freq, pre_gain);
		post_left_tone.setParams(post_freq, post_gain);
		post_right_tone.setParams(post_freq, post_gain);

		float left = inputs[LEFT_INPUT].getVoltage();
		left = pre_left_tone.process(left);
		left = 10.f*std::tanh(0.1f*drive*left);
		left = post_left_tone.process(left);
		outputs[LEFT_OUTPUT].setVoltage(left);

		float right = inputs[RIGHT_INPUT].getVoltage();
		right = pre_right_tone.process(right);
		right = 10.f*std::tanh(0.1f*drive*right);
		right = post_right_tone.process(right);
		outputs[RIGHT_OUTPUT].setVoltage(right);

	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		pre_left_tone = cs::Tilting<float>(e.sampleRate);
		post_left_tone = cs::Tilting<float>(e.sampleRate);
		pre_right_tone = cs::Tilting<float>(e.sampleRate);
		post_right_tone = cs::Tilting<float>(e.sampleRate);
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

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.05, 110.257)), module, TNT::RIGHT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 114.3)), module, TNT::LEFT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(42.143, 114.3)), module, TNT::RIGHT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.75, 120.65)), module, TNT::LEFT_OUTPUT));
	}
};


Model* modelTNT = createModel<TNT, TNTWidget>("TNT");