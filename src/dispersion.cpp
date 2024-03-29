#include "plugin.hpp"
#include "components/simple_svf.hpp"

using simd::float_4;

struct Dispersion : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		F_MOD_DEPTH_PARAM,
		Q_PARAM,
		Q_MOD_DEPTH_PARAM,
		DEPTH_PARAM,
		DRY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		F_MOD_INPUT,
		VPOCT_INPUT,
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

	#define M 32
	cs::SeriesAllpass<float_4, M> filter;

	Dispersion()
	: filter (cs::SeriesAllpass<float_4, M>(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQUENCY_PARAM, std::log2(10.f), std::log2(24000.f), std::log2(dsp::FREQ_C4), "Frequency", "Hz", 2);
		configParam(F_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configParam(Q_PARAM, 0.f, 1.f, 0.f, "Q");
		configParam(Q_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Q mod. depth");
		configParam(DEPTH_PARAM, 1.f, float(M), 1.f, "Depth");
		configParam(DRY_PARAM, 0.f, 1.f, 0.f, "Dry mix");
		configInput(F_MOD_INPUT, "Linear FM");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(Q_MOD_INPUT, "Q mod.");
		configInput(SIGNAL_INPUT, "Signal");
		configOutput(SIGNAL_OUTPUT, "Signal");
		getParamQuantity(DEPTH_PARAM)->snapEnabled = true;

		configBypass(SIGNAL_INPUT, SIGNAL_OUTPUT);
	}

	void process(const ProcessArgs& args) override {
		unsigned num_channels = std::max<unsigned>(inputs[SIGNAL_INPUT].getChannels(), inputs[VPOCT_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[F_MOD_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[Q_MOD_INPUT].getChannels());
		num_channels = std::min<unsigned>(num_channels, 4);
		outputs[SIGNAL_OUTPUT].setChannels(num_channels);

		float freq_knob = params[FREQUENCY_PARAM].getValue();
		float_4 vpoct = inputs[VPOCT_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 freq_tuning = dsp::approxExp2_taylor5(freq_knob + vpoct);
		float f_mod_depth = 5000.f * args.sampleTime * dsp::cubic(params[F_MOD_DEPTH_PARAM].getValue());
		float_4 f_mod = args.sampleRate * 0.1f * inputs[F_MOD_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 cutoff_param = freq_tuning + f_mod_depth * f_mod;
		float q_knob = dsp::quintic(params[Q_PARAM].getValue());
		float q_mod_depth = dsp::cubic(params[Q_MOD_DEPTH_PARAM].getValue());
		float_4 q_mod = 0.1f * inputs[Q_MOD_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 reso_param = float_4(q_knob + q_mod_depth * q_mod);
		reso_param = rescale(reso_param, 0.f, 1.f, 0.5f, 10.f);
		
		filter.setParams(cutoff_param, reso_param);
		float_4 in = inputs[SIGNAL_INPUT].getPolyVoltageSimd<float_4>(0);
		unsigned char depth = (unsigned)params[DEPTH_PARAM].getValue();
		filter.process(in, depth);
		float dry_level = params[DRY_PARAM].getValue();
		outputs[SIGNAL_OUTPUT].setVoltageSimd<float_4>(filter.getAllPass(depth) + dry_level * in, 0);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter = cs::SeriesAllpass<float_4, M>(e.sampleRate);
	}
};


struct DispersionWidget : ModuleWidget {
	DispersionWidget(Dispersion* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/dispersion.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 19.05)), module, Dispersion::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.476, 24.118)), module, Dispersion::F_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 57.15)), module, Dispersion::Q_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.474, 62.216)), module, Dispersion::Q_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 81.28)), module, Dispersion::DEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.321, 99.06)), module, Dispersion::DRY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.476, 16.506)), module, Dispersion::F_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 39.329)), module, Dispersion::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.474, 54.604)), module, Dispersion::Q_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 110.484)), module, Dispersion::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 110.486)), module, Dispersion::SIGNAL_OUTPUT));
	}
};


Model* modelDispersion = createModel<Dispersion, DispersionWidget>("Dispersion");