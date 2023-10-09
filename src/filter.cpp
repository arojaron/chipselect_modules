#include "plugin.hpp"

#include "components/simple_svf.hpp"
#include "components/tuned_envelope.hpp"

#include "components/sigmoid.hpp"

struct Filter : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		F_MOD_DEPTH_PARAM,
		Q_PARAM,
		Q_MOD_DEPTH_PARAM,
		RESO_MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		F_MOD_INPUT,
		Q_MOD_INPUT,
		PING_INPUT,
		VPOCT_INPUT,
		SIGNAL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		HIGH_PASS_OUTPUT,
		BAND_PASS_OUTPUT,
		LOW_PASS_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		RESO_MODE_LIGHT,
		LIGHTS_LEN
	};

	cs::SimpleSvf filter;
	cs::TunedDecayEnvelope<float> ping_processor;
	dsp::BooleanTrigger ping_trigger;

	bool reso_mode = false;

	Filter()
	: filter(cs::SimpleSvf(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RESO_MODE_PARAM, 0.f, 1.f, 0.f, "Resonator mode");
		configParam(FREQUENCY_PARAM, std::log2(10.f), std::log2(24000.f), std::log2(55.f), "Frequency", "Hz", 2);
		configParam(F_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configParam(Q_PARAM, 0.f, 1.f, 0.f, "Q");
		configParam(Q_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Q mod. depth");
		configInput(F_MOD_INPUT, "Linear FM");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(PING_INPUT, "Ping");
		configInput(Q_MOD_INPUT, "Q mod.");
		configInput(SIGNAL_INPUT, "Signal in");
		configOutput(HIGH_PASS_OUTPUT, "High pass");
		configOutput(BAND_PASS_OUTPUT, "Band pass");
		configOutput(LOW_PASS_OUTPUT, "Low pass");
	}

	void process(const ProcessArgs& args) override
	{
		reso_mode = params[RESO_MODE_PARAM].getValue() > 0.f;
		lights[RESO_MODE_LIGHT].setBrightness(reso_mode);

		float freq_knob = params[FREQUENCY_PARAM].getValue();
		float vpoct = inputs[VPOCT_INPUT].getVoltage();
		float freq_tuning = dsp::approxExp2_taylor5(freq_knob + vpoct);
		float f_mod_depth = 5000.f * args.sampleTime * dsp::cubic(params[F_MOD_DEPTH_PARAM].getValue());
		float f_mod = args.sampleRate * 0.1f * inputs[F_MOD_INPUT].getVoltage();
		float cutoff_param = freq_tuning + f_mod_depth * f_mod;

		float q_knob = dsp::quintic(params[Q_PARAM].getValue());
		float q_mod_depth = dsp::cubic(params[Q_MOD_DEPTH_PARAM].getValue());
		float q_mod = 0.1f * inputs[Q_MOD_INPUT].getVoltage();
		float reso_param = q_knob + q_mod_depth * q_mod;
		if(reso_mode){
			reso_param *= 10.f*freq_tuning;
		}
		else{
			reso_param = rescale(reso_param, 0.f, 1.f, 0.5f, 1000.f);
		}
		
		filter.setParams(cutoff_param, reso_param);

		ping_processor.setFrequency(cutoff_param);
		float ping_level = inputs[PING_INPUT].getVoltage();
		float triggered = ping_trigger.process(0.f < ping_level);
		float pulse = ping_level * ping_processor.process(args.sampleTime, triggered ? 1.f : 0.f);
		float in = inputs[SIGNAL_INPUT].getVoltage();
		
		outputs[LOW_PASS_OUTPUT].setVoltage(filter.process(in + pulse));
		outputs[BAND_PASS_OUTPUT].setVoltage(filter.getBandPass());
		outputs[HIGH_PASS_OUTPUT].setVoltage(filter.getHighPass());
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter = cs::SimpleSvf(e.sampleRate);
	}
};

struct FilterWidget : ModuleWidget {
	FilterWidget(Filter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/filter.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 15.856)), module, Filter::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 34.237)), module, Filter::F_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 52.917)), module, Filter::Q_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 71.298)), module, Filter::Q_MOD_DEPTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 34.237)), module, Filter::F_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 71.298)), module, Filter::Q_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 95.25)), module, Filter::PING_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 106.634)), module, Filter::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 118.019)), module, Filter::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 95.25)), module, Filter::HIGH_PASS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 106.634)), module, Filter::BAND_PASS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 118.019)), module, Filter::LOW_PASS_OUTPUT));

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<YellowLight>>>(mm2px(Vec(15.24, 83.274)), module, Filter::RESO_MODE_PARAM, Filter::RESO_MODE_LIGHT));
	}
};


Model* modelFilter = createModel<Filter, FilterWidget>("Filter");