#include "plugin.hpp"

#include "components/simple_svf.hpp"


struct Dispersion : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		F_MOD_DEPTH_PARAM,
		Q_PARAM,
		Q_MOD_DEPTH_PARAM,
		ORDER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		F_MOD_INPUT,
		Q_MOD_INPUT,
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

	cs::SimpleSvf filter1;
	cs::SimpleSvf filter2;

	Dispersion() 
	: filter1(cs::SimpleSvf(48000.f)), filter2(cs::SimpleSvf(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQUENCY_PARAM, std::log2(10.f), std::log2(24000.f), std::log2(55.f), "Frequency", "Hz", 2);
		configParam(F_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configParam(Q_PARAM, 0.f, 1.f, 0.f, "Q");
		configParam(Q_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Q mod. depth");
		configParam(ORDER_PARAM, 1.f, 2.f, 1.f, "Order");
		configInput(F_MOD_INPUT, "Linear FM");
		configInput(Q_MOD_INPUT, "Q mod.");
		configInput(SIGNAL_INPUT, "Signal");
		configOutput(SIGNAL_OUTPUT, "Signal");
	}

	void process(const ProcessArgs& args) override {
		float freq_knob = params[FREQUENCY_PARAM].getValue();
		float freq_tuning = dsp::approxExp2_taylor5(freq_knob);
		float f_mod_depth = 5000.f * args.sampleTime * dsp::cubic(params[F_MOD_DEPTH_PARAM].getValue());
		float f_mod = args.sampleRate * 0.1f * inputs[F_MOD_INPUT].getVoltage();
		float freq_param = freq_tuning + f_mod_depth * f_mod;

		float q_knob = dsp::quintic(params[Q_PARAM].getValue());
		float q_mod_depth = dsp::cubic(params[Q_MOD_DEPTH_PARAM].getValue());
		float q_mod = 0.1f * inputs[Q_MOD_INPUT].getVoltage();
		float reso_param = q_knob + q_mod_depth * q_mod;
		reso_param = rescale(reso_param, 0.f, 1.f, 0.5f, 10.f);
		
		filter1.setParams(freq_param, reso_param);
		filter2.setParams(freq_param, reso_param);

		float in = inputs[SIGNAL_INPUT].getVoltage();

		filter1.process(in);
		float all1 = filter1.getAllPass();
		filter2.process(all1);
		float all2 = filter2.getAllPass();

		outputs[SIGNAL_OUTPUT].setVoltage(all2);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter1 = cs::SimpleSvf(e.sampleRate);
		filter2 = cs::SimpleSvf(e.sampleRate);
	}
};


struct DispersionWidget : ModuleWidget {
	DispersionWidget(Dispersion* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/dispersion.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 15.856)), module, Dispersion::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 31.731)), module, Dispersion::F_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 52.917)), module, Dispersion::Q_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.873, 68.792)), module, Dispersion::Q_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 89.958)), module, Dispersion::ORDER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 31.731)), module, Dispersion::F_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 68.792)), module, Dispersion::Q_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 118.019)), module, Dispersion::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 118.019)), module, Dispersion::SIGNAL_OUTPUT));
	}
};


Model* modelDispersion = createModel<Dispersion, DispersionWidget>("Dispersion");