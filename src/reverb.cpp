#include "plugin.hpp"

#include "diffusion_stage.hpp"

simd::float_4 lensr[4] = {
	simd::float_4(0.0, 0.13231882452964783, 0.32141810655593872, 0.13058231770992279),
	simd::float_4(0.0, 0.655509352684021, 0.20878803730010986, 0.0019487274112179875),
	simd::float_4(0.0, 0.63371104001998901, 0.32119685411453247, 0.58446371555328369),
	simd::float_4(0.0, 0.73894518613815308, 0.64278435707092285, 0.54876101016998291)
};

simd::float_4 normsr[4] = {
	simd::float_4(-0.70389324426651001, 0.71581447124481201, 0.28890848159790039, 0.054718136787414551),
	simd::float_4(-0.13980937004089355, 0.34750247001647949, 0.45920848846435547, -0.1774132251739502),
	simd::float_4(0.89756679534912109, -0.24786150455474854, 0.14391446113586426, -0.12117087841033936),
	simd::float_4(0.88379204273223877, -0.17205017805099487, 0.62125051021575928, -0.30143225193023682),
};

struct Reverb : Module {
	enum ParamId {
		LENGTH_RATIO_C_PARAM,
		LENGTH_PARAM,
		LENGTH_RATIO_B_PARAM,
		LENGTH_RATIO_A_PARAM,
		DIFF_PARAM,
		DIFF_MODE_PARAM,
		FEEDBACK_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LEFT_INPUT,
		RIGHT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float FS = 48000.0;
	DiffusionStage4 diffusion1;
	DiffusionStage4 diffusion2;
	DelayStage4 delay;
	MatrixMixer4 mixer;

	Reverb() 
	: diffusion1(DiffusionStage4(lensr, normsr, FS)),
	  diffusion2(DiffusionStage4(lensr, normsr, FS)),
	  delay(DelayStage4(simd::float_4(1,1,1,1), FS)),
	  mixer(MatrixMixer4(simd::float_4(1,1,1,1)))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LENGTH_RATIO_C_PARAM, 0.f, 1.f, 0.f, "Length ratio C");
		configParam(LENGTH_PARAM, 0.f, 1.f, 0.f, "Length main", "s");
		configParam(LENGTH_RATIO_B_PARAM, 0.f, 1.f, 0.f, "Length ratio B");
		configParam(LENGTH_RATIO_A_PARAM, 0.f, 1.f, 0.f, "Length ratio A");
		configParam(DIFF_PARAM, 0.f, 1.f, 0.f, "Diffusion");
		configParam(DIFF_MODE_PARAM, 0.f, 1.f, 0.f, "Diffusion mode");
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Feedback");
		configInput(LEFT_INPUT, "Left");
		configInput(RIGHT_INPUT, "Right");
		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");
	}

	void process(const ProcessArgs& args) override
	{
		static simd::float_4 back_fed = simd::float_4::zero();

		float diff_depth = params[DIFF_PARAM].getValue();
		diff_depth = dsp::cubic(diff_depth);
		diffusion1.setScale(diff_depth);
		diffusion2.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);

		v = v + back_fed;
		v = diffusion1.process(v);
		v = diffusion2.process(v);

		outputs[LEFT_OUTPUT].setVoltage(v[0] + v[2]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1] + v[3]);

		float delay0 = params[LENGTH_PARAM].getValue();
		float delayA = delay0 * params[LENGTH_RATIO_A_PARAM].getValue();
		float delayB = delay0 * params[LENGTH_RATIO_B_PARAM].getValue();
		float delayC = delay0 * params[LENGTH_RATIO_C_PARAM].getValue();
		simd::float_4 delay_param = simd::float_4(delay0, delayA, delayB, delayC);
		delay_param = dsp::cubic(delay_param);
		delay.setScale(delay_param);

		v = mixer.process(delay.process(v));

		float feedback = params[FEEDBACK_PARAM].getValue();
		back_fed = v * simd::float_4(feedback);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		diffusion1 = DiffusionStage4(lensr, normsr, FS);
		diffusion2 = DiffusionStage4(lensr, normsr, FS);
	  	delay = DelayStage4(simd::float_4(1,1,1,1), FS);
	}
};

struct DiffModeButton : VCVButton{

};

struct ReverbWidget : ModuleWidget {
	ReverbWidget(Reverb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 15.171)), module, Reverb::LENGTH_RATIO_C_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(21.406, 24.806)), module, Reverb::LENGTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.822, 24.806)), module, Reverb::LENGTH_RATIO_B_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 34.442)), module, Reverb::LENGTH_RATIO_A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.867, 63.49)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(43.006, 63.49)), module, Reverb::DIFF_MODE_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(32.597, 93.356)), module, Reverb::FEEDBACK_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, Reverb::RIGHT_OUTPUT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("reverb");