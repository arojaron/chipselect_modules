#include "plugin.hpp"

#include "components/bandlimited_oscillator.hpp"


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

	bool sync_enabled = false;
	bool fm_enabled = false;

	dsp::BooleanTrigger reset_trigger;
	cs::BandlimitedSaw modulator;
	cs::BandlimitedSaw carrier;

	Sawtooth()
	{
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
		float modulator_signal = 0.f;
		float carrier_signal = 0.f;

		sync_enabled = params[SYNC_ENABLE_PARAM].getValue() > 0.f;
		lights[SYNC_ENABLE_LIGHT].setBrightness(sync_enabled);
		fm_enabled = params[FM_ENABLE_PARAM].getValue() > 0.f;
		lights[FM_ENABLE_LIGHT].setBrightness(fm_enabled);

		bool reset = reset_trigger.process(inputs[RESET_INPUT].getVoltage());

		if(outputs[MODULATOR_OUTPUT].isConnected() || sync_enabled || fm_enabled){
			float modulator_pitch = inputs[MODULATOR_VPOCT_INPUT].getVoltage();
			modulator_pitch +=  params[MODULATOR_OCT_PARAM].getValue();
			modulator_pitch += params[MODULATOR_TUNE_PARAM].getValue();
			float modulator_freq = dsp::approxExp2_taylor5(modulator_pitch);

			modulator.setFrequency(modulator_freq);
			if(reset){
				modulator.reset();
			}
			modulator_signal = modulator.process();
		}
		
		if(outputs[CARRIER_OUTPUT].isConnected()){
			float carrier_pitch = inputs[CARRIER_VPOCT_INPUT].getVoltage();
			carrier_pitch +=  params[CARRIER_OCT_PARAM].getValue();
			carrier_pitch += params[CARRIER_TUNE_PARAM].getValue();
			if(fm_enabled){
				float fm_depth = params[FM_DEPTH_PARAM].getValue() + 0.1f*inputs[FM_DEPTH_MOD_INPUT].getVoltage();
				float fm = fm_depth * modulator.getAliasedSawSample();
				carrier_pitch += fm;
			}
			float carrier_freq = dsp::approxExp2_taylor5(carrier_pitch);
			carrier.setFrequency(carrier_freq);
			if(reset){
				carrier.reset();
			}
			else if(sync_enabled && modulator.overturned){
				carrier.sync(modulator.overturn_delay);
			}
			carrier_signal = carrier.process();
		}

		outputs[MODULATOR_OUTPUT].setVoltage(modulator_signal);
		outputs[CARRIER_OUTPUT].setVoltage(carrier_signal);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		modulator.setSampleTime(e.sampleTime);
		carrier.setSampleTime(e.sampleTime);
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
};

Model* modelSawtooth = createModel<Sawtooth, SawtoothWidget>("Sawtooth");
