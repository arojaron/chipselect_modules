#include "plugin.hpp"

#include "components/matched_biquad.hpp"
#include "components/tuned_envelope.hpp"


struct LowPass : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		F_MOD_DEPTH_PARAM,
		Q_PARAM,
		Q_MOD_DEPTH_PARAM,
		RESONATOR_MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		F_MOD_INPUT,
		Q_MOD_INPUT,
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
		RESONATOR_MODE_LIGHT,
		LIGHTS_LEN
	};

	cs::LowPass<float> filter;
	cs::TunedDecayEnvelope<float> ping_processor;
	dsp::BooleanTrigger ping_trigger;

	bool res_mode = false;

	LowPass()
	: filter(cs::LowPass<float>(48000))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQUENCY_PARAM, std::log2(10.f), std::log2(24000.f), std::log2(55.f), "Frequency", "Hz", 2);
		configParam(F_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configParam(Q_PARAM, 0.f, 1.f, 0.f, "Q");
		configParam(Q_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Q mod. depth");
		configParam(RESONATOR_MODE_PARAM, 0.f, 1.f, 0.f, "Resonator mode on");
		configInput(F_MOD_INPUT, "Linear FM");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(PING_INPUT, "Ping");
		configInput(Q_MOD_INPUT, "Q mod.");
		configInput(FILTER_INPUT, "Filter");
		configOutput(FILTER_OUTPUT, "Filter");
	}

	void process(const ProcessArgs& args) override
	{
		float freq_knob = params[FREQUENCY_PARAM].getValue();
		float vpoct = inputs[VPOCT_INPUT].getVoltage();
		float freq_tuning = dsp::approxExp2_taylor5(freq_knob + vpoct);
		float f_mod_depth = 5000.f * args.sampleTime * dsp::quintic(params[F_MOD_DEPTH_PARAM].getValue());
		float f_mod = args.sampleRate * 0.1f * inputs[F_MOD_INPUT].getVoltage();
		float cutoff_param = freq_tuning + f_mod_depth * f_mod;

		float q_knob = dsp::cubic(params[Q_PARAM].getValue());
		float q_mod_depth = dsp::cubic(params[Q_MOD_DEPTH_PARAM].getValue());
		float q_mod = 0.1f * inputs[Q_MOD_INPUT].getVoltage();
		float reso_param = q_knob + q_mod_depth * q_mod;

		if(res_mode){
			reso_param *= 0.5f * freq_tuning;
		}
		else{
			reso_param = rescale(reso_param, 0.f, 1.f, 0.5f, 50.f);
		}
		
		filter.setParams(cutoff_param, reso_param);

		ping_processor.setFrequency(cutoff_param);
		float ping_level = inputs[PING_INPUT].getVoltage();
		float triggered = ping_trigger.process(0.f < ping_level);
		float pulse = ping_level * ping_processor.process(args.sampleTime, triggered ? 1.f : 0.f);
		float in = inputs[FILTER_INPUT].getVoltage();
		
		outputs[FILTER_OUTPUT].setVoltage(filter.process(in + pulse));
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter = cs::LowPass<float>(e.sampleRate);
	}

	void resModeToggle(void)
	{
		res_mode = !res_mode;
		lights[RESONATOR_MODE_LIGHT].setBrightness(res_mode? 1.f : 0.f);
	}
};

struct ResModeButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		VCVButton::onDragStart(e);
		((LowPass*)this->module)->resModeToggle();
	}
};

struct LowPassWidget : ModuleWidget {
	LowPassWidget(LowPass* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/low_pass.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 15.856)), module, LowPass::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 34.237)), module, LowPass::F_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 63.908)), module, LowPass::Q_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 82.289)), module, LowPass::Q_MOD_DEPTH_PARAM));
		addParam(createParamCentered<ResModeButton>(mm2px(Vec(15.24, 95.468)), module, LowPass::RESONATOR_MODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 34.237)), module, LowPass::F_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 82.289)), module, LowPass::Q_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 106.757)), module, LowPass::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.873, 106.757)), module, LowPass::PING_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 118.019)), module, LowPass::FILTER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 118.019)), module, LowPass::FILTER_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(23.431, 95.468)), module, LowPass::RESONATOR_MODE_LIGHT));
	}
};


Model* modelLowPass = createModel<LowPass, LowPassWidget>("LowPass");