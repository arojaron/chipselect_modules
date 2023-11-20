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

	bool sync_enabled = false;
	bool fm_enabled = false;

	dsp::BooleanTrigger reset_trigger[4];
	cs::BandlimitedSaw modulator[4];
	cs::BandlimitedSaw carrier[4];

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

			for(unsigned i = 0; i < 4; i++) {
				modulator[i].setFrequency(modulator_freq[i]);
				if(reset[i]){
					modulator[i].reset();
				}
				modulator_signal[i] = modulator[i].process();
			}
		}
		
		if(outputs[CARRIER_OUTPUT].isConnected()){
			float_4 carrier_pitch = inputs[CARRIER_VPOCT_INPUT].getPolyVoltageSimd<float_4>(0);
			carrier_pitch +=  params[CARRIER_OCT_PARAM].getValue();
			carrier_pitch += params[CARRIER_TUNE_PARAM].getValue();
			if(fm_enabled){
				float fm_depth = params[FM_DEPTH_PARAM].getValue() + 0.1f*inputs[FM_DEPTH_MOD_INPUT].getVoltage();
				float_4 fm;
				for(unsigned i = 0; i < 4; i++) {
					fm[i] = fm_depth * modulator[i].getAliasedSawSample();
				}
				carrier_pitch += fm;
			}
			float_4 carrier_freq = dsp::approxExp2_taylor5(carrier_pitch);
			for(unsigned i = 0; i < 4; i++) {
				carrier[i].setFrequency(carrier_freq[i]);
				if(reset[i]){
					carrier[i].reset();
				}
				else if(sync_enabled && modulator[i].overturned){
					carrier[i].sync(modulator[i].overturn_delay);
				}
				carrier_signal[i] = carrier[i].process();
			}
		}

		outputs[MODULATOR_OUTPUT].setVoltageSimd(modulator_signal, 0);
		outputs[CARRIER_OUTPUT].setVoltageSimd(carrier_signal, 0);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		for(unsigned i = 0; i < 4; i++) {
			modulator[i].setSampleTime(e.sampleTime);
			carrier[i].setSampleTime(e.sampleTime);
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
};

Model* modelSawtooth = createModel<Sawtooth, SawtoothWidget>("Sawtooth");
