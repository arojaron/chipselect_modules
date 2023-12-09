#include "plugin.hpp"

#include "components/simple_svf.hpp"
#include "components/tuned_envelope.hpp"

using simd::float_4;

struct Filter : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		F_MOD_DEPTH_PARAM,
		Q_PARAM,
		Q_MOD_DEPTH_PARAM,
		RESO_MODE_PARAM,
		DRY_PARAM,
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

	enum NumOfPoles {
		FOUR = 0,
		TWO = 1,
		NUM_OF_POLES_LEN
	};
	unsigned num_of_poles = TWO;

	cs::SimpleSvf<float_4> filter_base;
	cs::SimpleSvf<float_4> filter_low;
	cs::SimpleSvf<float_4> filter_band;
	cs::SimpleSvf<float_4> filter_high;
	cs::TriggerProcessor<float_4> ping_trigger;
	cs::TunedDecayEnvelope<float_4> ping_envelope;

	Filter()
	: filter_base(cs::SimpleSvf<float_4>(48000.f)), 
	  filter_low(cs::SimpleSvf<float_4>(48000.f)), 
	  filter_band(cs::SimpleSvf<float_4>(48000.f)), 
	  filter_high(cs::SimpleSvf<float_4>(48000.f))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RESO_MODE_PARAM, 0.f, 1.f, 0.f, "Resonator mode");
		configParam(FREQUENCY_PARAM, std::log2(10.f), std::log2(24000.f), std::log2(dsp::FREQ_C4), "Frequency", "Hz", 2);
		configParam(F_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configParam(Q_PARAM, 0.f, 1.f, 0.f, "Q");
		configParam(Q_MOD_DEPTH_PARAM, -1.f, 1.f, 0.f, "Q mod. depth");
		configParam(DRY_PARAM, 0.f, 1.f, 0.f, "Dry mix");
		configInput(F_MOD_INPUT, "Linear FM");
		configInput(VPOCT_INPUT, "V/Oct");
		configInput(PING_INPUT, "Ping");
		configInput(Q_MOD_INPUT, "Q mod.");
		configInput(SIGNAL_INPUT, "Signal in");
		configOutput(HIGH_PASS_OUTPUT, "High pass");
		configOutput(BAND_PASS_OUTPUT, "Band pass");
		configOutput(LOW_PASS_OUTPUT, "Low pass");

		configBypass(SIGNAL_INPUT, LOW_PASS_OUTPUT);
		configBypass(SIGNAL_INPUT, BAND_PASS_OUTPUT);
		configBypass(SIGNAL_INPUT, HIGH_PASS_OUTPUT);
	}

	void process(const ProcessArgs& args) override
	{
		unsigned num_channels = std::max<unsigned>(inputs[SIGNAL_INPUT].getChannels(), inputs[VPOCT_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[F_MOD_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[Q_MOD_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[PING_INPUT].getChannels());
		num_channels = std::min<unsigned>(num_channels, 4);
		outputs[LOW_PASS_OUTPUT].setChannels(num_channels);
		outputs[BAND_PASS_OUTPUT].setChannels(num_channels);
		outputs[HIGH_PASS_OUTPUT].setChannels(num_channels);

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

		bool reso_mode = params[RESO_MODE_PARAM].getValue() > 0.f;
		lights[RESO_MODE_LIGHT].setBrightness(reso_mode);
		if(reso_mode){
			reso_param *= 10.f*freq_tuning;
		}
		else{
			reso_param = rescale(reso_param, 0.f, 1.f, 0.5f, 1000.f);
		}
		ping_envelope.setFrequency(cutoff_param);
		float_4 ping_level = inputs[PING_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 pulse = ping_envelope.process(args.sampleTime, ping_trigger.process(ping_level));
		float_4 in = inputs[SIGNAL_INPUT].getPolyVoltageSimd<float_4>(0);
		float_4 dry = float_4(params[DRY_PARAM].getValue());

		switch(num_of_poles){
			default:
			case FOUR:
				reso_param = simd::ifelse(reso_param < 0.f, 0.f, simd::sqrt(reso_param));
				filter_base.setParams(cutoff_param, reso_param);
				filter_base.process(in);
				if(outputs[LOW_PASS_OUTPUT].isConnected()){
					filter_low.setParams(cutoff_param, reso_param);
					filter_low.process(filter_base.getLowPass() + pulse);
					outputs[LOW_PASS_OUTPUT].setVoltageSimd<float_4>(filter_low.getLowPass() + dry*in, 0);
				}
				if(outputs[BAND_PASS_OUTPUT].isConnected()){
					filter_band.setParams(cutoff_param, reso_param);
					filter_band.process(2.f*filter_base.getBandPass() + pulse);
					outputs[BAND_PASS_OUTPUT].setVoltageSimd<float_4>(filter_band.getBandPass() + dry*in, 0);
				}
				if(outputs[HIGH_PASS_OUTPUT].isConnected()){
					filter_high.setParams(cutoff_param, reso_param);
					filter_high.process(filter_base.getHighPass() + pulse);
					outputs[HIGH_PASS_OUTPUT].setVoltageSimd<float_4>(filter_high.getHighPass() + dry*in, 0);
				}
			break;
			case TWO:
				filter_base.setParams(cutoff_param, reso_param);
				filter_base.process(in + pulse);
				outputs[LOW_PASS_OUTPUT].setVoltageSimd<float_4>(filter_base.getLowPass() + dry*in, 0);
				outputs[BAND_PASS_OUTPUT].setVoltageSimd<float_4>(simd::sqrt(2.f)*filter_base.getBandPass() + dry*in, 0);
				outputs[HIGH_PASS_OUTPUT].setVoltageSimd<float_4>(filter_base.getHighPass() + dry*in, 0);
			break;
		}
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter_base = cs::SimpleSvf<float_4>(e.sampleRate);
		filter_low = cs::SimpleSvf<float_4>(e.sampleRate);
		filter_band = cs::SimpleSvf<float_4>(e.sampleRate);
		filter_high = cs::SimpleSvf<float_4>(e.sampleRate);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "num_of_poles", json_integer(num_of_poles));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "num_of_poles");
		if (modeJ) {
			num_of_poles = json_integer_value(modeJ);
		}
	}
};

struct FilterWidget : ModuleWidget {
	FilterWidget(Filter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/filter.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 19.05)), module, Filter::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.476, 24.118)), module, Filter::F_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 57.15)), module, Filter::Q_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.474, 62.216)), module, Filter::Q_MOD_DEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.241, 93.98)), module, Filter::DRY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.476, 16.506)), module, Filter::F_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 39.329)), module, Filter::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.474, 54.604)), module, Filter::Q_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 110.484)), module, Filter::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 110.464)), module, Filter::PING_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 82.509)), module, Filter::HIGH_PASS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 96.479)), module, Filter::BAND_PASS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 110.486)), module, Filter::LOW_PASS_OUTPUT));

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(15.24, 76.2)), module, Filter::RESO_MODE_PARAM, Filter::RESO_MODE_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Filter* module = dynamic_cast<Filter*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("Mode"));

		struct ModeItem : MenuItem {
			Filter* module;
			int poles;
			void onAction(const event::Action& e) override {
				module->num_of_poles = poles;
			}
		};

		std::string polesNames[2] = {"Four pole", "Two pole"};
		for (unsigned i = 0; i < Filter::NUM_OF_POLES_LEN; i++) {
			ModeItem* modeItem = createMenuItem<ModeItem>(polesNames[i]);
			modeItem->rightText = CHECKMARK(module->num_of_poles == i);
			modeItem->module = module;
			modeItem->poles = i;
			menu->addChild(modeItem);
		}
	}
};


Model* modelFilter = createModel<Filter, FilterWidget>("Filter");