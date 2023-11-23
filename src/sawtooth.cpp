#include "plugin.hpp"

#include "components/bandlimited_oscillator.hpp"

using namespace simd;

struct Sawtooth : Module {
	enum ParamId {
		MODULATOR_OCT_PARAM,
		MODULATOR_TUNE_PARAM,
		FM_DEPTH_PARAM,
		SYNC_ENABLE_PARAM,
		FM_ENABLE_PARAM,
		CARRIER_OCT_PARAM,
		CARRIER_TUNE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MODULATOR_VPOCT_INPUT,
		RESET_INPUT,
		FM_DEPTH_MOD_INPUT,
		CARRIER_VPOCT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MODULATOR_OUTPUT,
		CARRIER_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		SYNC_ENABLE_LIGHT,
		FM_ENABLE_LIGHT,
		LIGHTS_LEN
	};

	enum OscillatorType {
		SAWTOOTH,
		TRIANGLE,
		OSCILLATOR_TYPE_LEN
	};

	unsigned modulator_type = SAWTOOTH;
	unsigned carrier_type = SAWTOOTH;

	bool sync_enabled = false;
	bool fm_enabled = false;

	dsp::BooleanTrigger reset_trigger[4];
	cs::Triangle tri_A[4];
	cs::Triangle tri_B[4];
	cs::Saw saw_A[4];
	cs::Saw saw_B[4];

	Sawtooth() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MODULATOR_OCT_PARAM, -5.f, 5.f, 0.f, "Modulator octave");
		getParamQuantity(MODULATOR_OCT_PARAM)->snapEnabled = true;
		configParam(MODULATOR_TUNE_PARAM, std::log2(dsp::FREQ_C4/32.f), std::log2(dsp::FREQ_C4*32.f), std::log2(dsp::FREQ_C4), "Modulator tune", "Hz", 2);
		configParam(FM_DEPTH_PARAM, -1.f, 1.f, 0.f, "FM depth");
		configSwitch(SYNC_ENABLE_PARAM, 0.f, 1.f, 0.f, "Hard snyc enable");
		configSwitch(FM_ENABLE_PARAM, 0.f, 1.f, 0.f, "FM enable");
		configParam(CARRIER_OCT_PARAM, -5.f, 5.f, 0.f, "Carrier octave");
		getParamQuantity(CARRIER_OCT_PARAM)->snapEnabled = true;
		configParam(CARRIER_TUNE_PARAM, std::log2(dsp::FREQ_C4/32.f), std::log2(dsp::FREQ_C4*32.f), std::log2(dsp::FREQ_C4), "Carrier tune", "Hz", 2);
		configInput(MODULATOR_VPOCT_INPUT, "Modulator V/Oct");
		configInput(RESET_INPUT, "Reset");
		configInput(FM_DEPTH_MOD_INPUT, "FM depth mod.");
		configInput(CARRIER_VPOCT_INPUT, "Carrier V/Oct");
		configOutput(MODULATOR_OUTPUT, "Modulator");
		configOutput(CARRIER_OUTPUT, "Carrier");
	}

	void process(const ProcessArgs& args) override {
		unsigned num_channels = std::max<unsigned>(inputs[MODULATOR_VPOCT_INPUT].getChannels(), inputs[CARRIER_VPOCT_INPUT].getChannels());
		num_channels = std::max<unsigned>(num_channels, inputs[RESET_INPUT].getChannels());
		num_channels = std::min<unsigned>(num_channels, 4);
		outputs[MODULATOR_OUTPUT].setChannels(num_channels);
		outputs[CARRIER_OUTPUT].setChannels(num_channels);

		float_4 modulator_signal = 0.f;
		float_4 carrier_signal = 0.f;

		sync_enabled = params[SYNC_ENABLE_PARAM].getValue() > 0.f;
		lights[SYNC_ENABLE_LIGHT].setBrightness(sync_enabled);
		fm_enabled = params[FM_ENABLE_PARAM].getValue() > 0.f;
		lights[FM_ENABLE_LIGHT].setBrightness(fm_enabled);

		bool reset[4];
		for(unsigned i = 0; i < 4; i++){
			reset[i] = reset_trigger[i].process(inputs[RESET_INPUT].getVoltage(i));
		}

		if(outputs[MODULATOR_OUTPUT].isConnected() || sync_enabled || fm_enabled){
			float_4 modulator_pitch = inputs[MODULATOR_VPOCT_INPUT].getPolyVoltageSimd<float_4>(0);
			modulator_pitch +=  params[MODULATOR_OCT_PARAM].getValue();
			modulator_pitch += params[MODULATOR_TUNE_PARAM].getValue();
			float_4 modulator_freq = dsp::approxExp2_taylor5(modulator_pitch);

			switch(modulator_type){
				case SAWTOOTH:
				for(unsigned i = 0; i < 4; i++) {
					saw_A[i].setFrequency(modulator_freq[i]);
					if(reset[i]){
						saw_A[i].reset();
					}
					modulator_signal[i] = saw_A[i].process();
				}
				break;
				case TRIANGLE:
				for(unsigned i = 0; i < 4; i++) {
					tri_A[i].setFrequency(modulator_freq[i]);
					if(reset[i]){
						tri_A[i].reset();
					}
					modulator_signal[i] = tri_A[i].process();
				}
				break;
			};
		}
		
		if(outputs[CARRIER_OUTPUT].isConnected()){
			float_4 carrier_pitch = inputs[CARRIER_VPOCT_INPUT].getPolyVoltageSimd<float_4>(0);
			carrier_pitch +=  params[CARRIER_OCT_PARAM].getValue();
			carrier_pitch += params[CARRIER_TUNE_PARAM].getValue();
			if(fm_enabled){
				float fm_depth = params[FM_DEPTH_PARAM].getValue() + 0.1f*inputs[FM_DEPTH_MOD_INPUT].getVoltage();
				float_4 fm;
				switch(modulator_type){
					case SAWTOOTH:
					for(unsigned i = 0; i < 4; i++) {
						fm[i] = fm_depth * saw_A[i].getAliasedSample();
					}
					break;
					case TRIANGLE:
					for(unsigned i = 0; i < 4; i++) {
						fm[i] = fm_depth * tri_A[i].getAliasedSample();
					}
					break;
				};
				carrier_pitch += fm;
			}
			float_4 carrier_freq = dsp::approxExp2_taylor5(carrier_pitch);
			switch(carrier_type){
				case SAWTOOTH:
				for(unsigned i = 0; i < 4; i++) {
					saw_B[i].setFrequency(carrier_freq[i]);
					if(reset[i]){
						saw_B[i].reset();
					}
					else if(sync_enabled){
						switch(modulator_type){
							case SAWTOOTH:
							if(saw_A[i].overturned){
								saw_B[i].sync(saw_A[i].overturn_delay);
							}
							break;
							case TRIANGLE:
							if(tri_A[i].overturned){
								saw_B[i].sync(tri_A[i].overturn_delay);
							}
							break;
						};
					}
					carrier_signal[i] = saw_B[i].process();
				}
				break;
				case TRIANGLE:
				for(unsigned i = 0; i < 4; i++) {
					tri_B[i].setFrequency(carrier_freq[i]);
					if(reset[i]){
						tri_B[i].reset();
					}
					else if(sync_enabled){
						switch(modulator_type){
							case SAWTOOTH:
							if(saw_A[i].overturned){
								tri_B[i].sync(saw_A[i].overturn_delay);
							}
							break;
							case TRIANGLE:
							if(tri_A[i].overturned){
								tri_B[i].sync(tri_A[i].overturn_delay);
							}
							break;
						};
					}
					carrier_signal[i] = tri_B[i].process();
				}
				break;
			};
		}

		outputs[MODULATOR_OUTPUT].setVoltageSimd(modulator_signal, 0);
		outputs[CARRIER_OUTPUT].setVoltageSimd(carrier_signal, 0);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		for(unsigned i = 0; i < 4; i++) {
			tri_A[i].setSampleTime(e.sampleTime);
			tri_B[i].setSampleTime(e.sampleTime);
			saw_A[i].setSampleTime(e.sampleTime);
			saw_B[i].setSampleTime(e.sampleTime);		
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "modulator_type", json_integer(modulator_type));
		json_object_set_new(rootJ, "carrier_type", json_integer(carrier_type));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modTypeJ = json_object_get(rootJ, "modulator_type");
		if (modTypeJ) {
			modulator_type = json_integer_value(modTypeJ);
		}
		json_t* carTypeJ = json_object_get(rootJ, "carrier_type");
		if (carTypeJ) {
			carrier_type = json_integer_value(carTypeJ);
		}
	}
};

struct SawtoothWidget : ModuleWidget {
	SawtoothWidget(Sawtooth* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/sawtooth.svg")));

		addParam(createParamCentered<VCVSlider>(mm2px(Vec(10.16, 29.21)), module, Sawtooth::MODULATOR_OCT_PARAM));
		addParam(createParamCentered<VCVSlider>(mm2px(Vec(20.32, 29.21)), module, Sawtooth::MODULATOR_TUNE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.48, 54.61)), module, Sawtooth::FM_DEPTH_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(20.32, 63.5)), module, Sawtooth::SYNC_ENABLE_PARAM, Sawtooth::SYNC_ENABLE_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(30.48, 63.5)), module, Sawtooth::FM_ENABLE_PARAM, Sawtooth::FM_ENABLE_LIGHT));
		addParam(createParamCentered<VCVSlider>(mm2px(Vec(10.16, 100.33)), module, Sawtooth::CARRIER_OCT_PARAM));
		addParam(createParamCentered<VCVSlider>(mm2px(Vec(20.32, 100.33)), module, Sawtooth::CARRIER_TUNE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 20.32)), module, Sawtooth::MODULATOR_VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 63.5)), module, Sawtooth::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 77.47)), module, Sawtooth::FM_DEPTH_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 91.44)), module, Sawtooth::CARRIER_VPOCT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 34.29)), module, Sawtooth::MODULATOR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 105.41)), module, Sawtooth::CARRIER_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Sawtooth* module = dynamic_cast<Sawtooth*>(this->module);
		std::string types_names[Sawtooth::OscillatorType::OSCILLATOR_TYPE_LEN] = {"Sawtooth", "Triangle"};

		menu->addChild(createMenuLabel("Modulator type"));
		struct ModTypeItem : MenuItem {
			Sawtooth* module;
			unsigned type;
			void onAction(const event::Action& e) override {
				module->modulator_type = type;
			}
		};
		for (unsigned i = 0; i < Sawtooth::OscillatorType::OSCILLATOR_TYPE_LEN; i++) {
			ModTypeItem* type_item = createMenuItem<ModTypeItem>(types_names[i]);
			type_item->rightText = CHECKMARK(module->modulator_type == i);
			type_item->module = module;
			type_item->type = i;
			menu->addChild(type_item);
		}

		menu->addChild(createMenuLabel("Carrier type"));
		struct CarTypeItem : MenuItem {
			Sawtooth* module;
			unsigned type;
			void onAction(const event::Action& e) override {
				module->carrier_type = type;
			}
		};
		for (unsigned i = 0; i < Sawtooth::OscillatorType::OSCILLATOR_TYPE_LEN; i++) {
			CarTypeItem* type_item = createMenuItem<CarTypeItem>(types_names[i]);
			type_item->rightText = CHECKMARK(module->carrier_type == i);
			type_item->module = module;
			type_item->type = i;
			menu->addChild(type_item);
		}
	}
};

Model* modelSawtooth = createModel<Sawtooth, SawtoothWidget>("Sawtooth");
