#include "plugin.hpp"

#include "components/simple_svf.hpp"
#include "components/tuned_envelope.hpp"

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
	unsigned num_of_poles = FOUR;

	cs::SimpleSvf<float> filter_base;
	cs::SimpleSvf<float> filter_low;
	cs::SimpleSvf<float> filter_band;
	cs::SimpleSvf<float> filter_high;
	cs::TunedDecayEnvelope<float> ping_processor;
	dsp::BooleanTrigger ping_trigger;

	bool reso_mode = false;

	Filter()
	: filter_base(cs::SimpleSvf<float>(48000.f)), 
	  filter_low(cs::SimpleSvf<float>(48000.f)), 
	  filter_band(cs::SimpleSvf<float>(48000.f)), 
	  filter_high(cs::SimpleSvf<float>(48000.f))
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
		ping_processor.setFrequency(cutoff_param);
		float ping_level = inputs[PING_INPUT].getVoltage();
		float triggered = ping_trigger.process(0.f < ping_level);
		float pulse = ping_level * ping_processor.process(args.sampleTime, triggered ? 1.f : 0.f);
		float in = inputs[SIGNAL_INPUT].getVoltage();

		float dry = params[DRY_PARAM].getValue();

		switch(num_of_poles){
			default:
			case FOUR:
				reso_param = (reso_param < 0.f) ? 0.f : simd::sqrt(reso_param);
				filter_base.setParams(cutoff_param, reso_param);
				filter_base.process(in + pulse);
				if(outputs[LOW_PASS_OUTPUT].isConnected()){
					filter_low.setParams(cutoff_param, reso_param);
					filter_low.process(filter_base.getLowPass());
					outputs[LOW_PASS_OUTPUT].setVoltage(filter_low.getLowPass() + dry*in);
				}
				if(outputs[BAND_PASS_OUTPUT].isConnected()){
					filter_band.setParams(cutoff_param, reso_param);
					filter_band.process(2.f*filter_base.getBandPass());
					outputs[BAND_PASS_OUTPUT].setVoltage(filter_band.getBandPass() + dry*in);
				}
				if(outputs[HIGH_PASS_OUTPUT].isConnected()){
					filter_high.setParams(cutoff_param, reso_param);
					filter_high.process(filter_base.getHighPass());
					outputs[HIGH_PASS_OUTPUT].setVoltage(filter_high.getHighPass() + dry*in);
				}
			break;
			case TWO:
				filter_base.setParams(cutoff_param, reso_param);
				filter_base.process(in + pulse);
				outputs[LOW_PASS_OUTPUT].setVoltage(filter_base.getLowPass() + dry*in);
				outputs[BAND_PASS_OUTPUT].setVoltage(simd::sqrt(2.f)*filter_base.getBandPass() + dry*in);
				outputs[HIGH_PASS_OUTPUT].setVoltage(filter_base.getHighPass() + dry*in);
			break;
		}
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		filter_base = cs::SimpleSvf<float>(e.sampleRate);
		filter_low = cs::SimpleSvf<float>(e.sampleRate);
		filter_band = cs::SimpleSvf<float>(e.sampleRate);
		filter_high = cs::SimpleSvf<float>(e.sampleRate);
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