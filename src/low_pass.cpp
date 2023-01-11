#include "plugin.hpp"

#include "components/matched_biquad.hpp"


struct LowPass : Module {
	enum ParamId {
		CUTOFF_PARAM,
		LFM_DEPTH_PARAM,
		RESONANCE_PARAM,
		RES_MOD_DEPTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LFM_INPUT,
		RES_MOD_INPUT,
		VPOCT_INPUT,
		PING_INPUT,
		FILTER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		FILTER_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	cs::LowPass<float> filter;

	LowPass()
	: filter(cs::LowPass<float>(48000))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CUTOFF_PARAM, std::log2(10), std::log2(21000), std::log2(dsp::FREQ_A4), "Frequency", "Hz", 2);
		configParam(LFM_DEPTH_PARAM, -1.f, 1.f, 0.f, "LFM depth");
		configParam(RESONANCE_PARAM, 0, 1, 0, "Resonance");
		configParam(RES_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Res. mod. depth");
		configInput(LFM_INPUT, "Linear FM");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(PING_INPUT, "Ping");
		configInput(RES_MOD_INPUT, "Resonance mod.");
		configInput(FILTER_INPUT, "Filter");
		configOutput(FILTER_OUTPUT, "Filter");
	}

	void process(const ProcessArgs& args) override
	{
		float freq_knob = params[CUTOFF_PARAM].getValue();
		float vpoct = inputs[VPOCT_INPUT].getVoltage();
		float linfm_depth = dsp::cubic(params[LFM_DEPTH_PARAM].getValue());
		float linfm = args.sampleRate*0.1*inputs[LFM_INPUT].getVoltage();
		float cutoff_param = dsp::approxExp2_taylor5(freq_knob + vpoct) + linfm_depth*linfm;

		float res_knob = dsp::quintic(params[RESONANCE_PARAM].getValue());
		float res_mod_depth = dsp::cubic(params[RES_MOD_DEPTH_PARAM].getValue());
		float res_mod = 0.1*inputs[RES_MOD_INPUT].getVoltage();
		float reso_param = res_knob + res_mod_depth*res_mod;

		reso_param = rescale(reso_param, 0, 1, 0.5, 1000);

		filter.setParams(cutoff_param, reso_param);

		float in = inputs[FILTER_INPUT].getVoltage();
		outputs[FILTER_OUTPUT].setVoltage(filter.process(in));
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter = cs::LowPass<float>(e.sampleRate);
	}
};


struct LowPassWidget : ModuleWidget {
	LowPassWidget(LowPass* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/low_pass.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 15.856)), module, LowPass::CUTOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 34.237)), module, LowPass::LFM_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 63.908)), module, LowPass::RESONANCE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 82.289)), module, LowPass::RES_MOD_DEPTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 34.237)), module, LowPass::LFM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 82.289)), module, LowPass::RES_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 106.757)), module, LowPass::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.873, 106.757)), module, LowPass::PING_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 118.019)), module, LowPass::FILTER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 118.019)), module, LowPass::FILTER_OUTPUT));
	}
};


Model* modelLowPass = createModel<LowPass, LowPassWidget>("LowPass");