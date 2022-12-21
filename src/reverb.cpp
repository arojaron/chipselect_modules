#include "plugin.hpp"
#include "diffusion_stage.hpp"
#include "reverb_constants.hpp"


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

	unsigned diff_mode = 0;
	float FS = 48000.0;
	DiffusionStage4 diff_stage;
	DelayStage4 delay_stage;
	MatrixMixer4 delay_mixer;

	Reverb()
	: diff_stage(DiffusionStage4(lengths_A, mix_coefs_N, FS)),
	  delay_stage(DelayStage4(lengths_K, FS)),
	  delay_mixer(MatrixMixer4(mix_coefs_K))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LENGTH_RATIO_C_PARAM, 0.f, 1.f, 1.f, "");
		configParam(LENGTH_PARAM, 0.01f, 1.f, 1.f, "Main delay length", "s");
		configParam(LENGTH_RATIO_B_PARAM, 0.f, 1.f, 1.f, "");
		configParam(LENGTH_RATIO_A_PARAM, 0.f, 1.f, 1.f, "");
		configParam(DIFF_PARAM, 0.f, 1.f, 0.f, "Diffusion depth");
		configParam(DIFF_MODE_PARAM, 0.f, 1.f, 0.f, "Diff. mode");
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
		diff_stage.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);
		
		v = diff_stage.process(v + back_fed);

		outputs[LEFT_OUTPUT].setVoltage(v[0]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1]);

		float delay_scale = params[LENGTH_PARAM].getValue();
		delay_scale = dsp::cubic(delay_scale);
		delay_stage.setScale(delay_scale);

		//v = delay_mixer.process(delay_stage.process(v));

		float feedback = params[FEEDBACK_PARAM].getValue();
		back_fed = simd::float_4(feedback)*v;
	}

	void diffModeSelect(void)
	{
		switch(diff_mode){
		case 0:
			diff_stage = DiffusionStage4(lengths_A, mix_coefs_N, FS);
			break;
		case 1:
			diff_stage = DiffusionStage4(lengths_B, mix_coefs_N, FS);
			break;
		case 2:
			diff_stage = DiffusionStage4(lengths_C, mix_coefs_N, FS);
			break;
		case 3:
			diff_stage = DiffusionStage4(lengths_D, mix_coefs_N, FS);
			break;
		default:
			break;
		};
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		diffModeSelect();
		delay_stage = DelayStage4(lengths_K, FS);
	}
	void onDiffModeChange(void)
	{
		diff_mode++;
		if(diff_mode >= 4) diff_mode = 0;
		diffModeSelect();
	}
};

struct DiffModeButton : VCVButton {
	void onDragStart(const DragStartEvent& e) override
	{
		((Reverb*)this->module)->onDiffModeChange();
		VCVButton::onDragStart(e);
	}
};


struct ReverbWidget : ModuleWidget {
	ReverbWidget(Reverb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb.svg")));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(37.854, 15.171)), module, Reverb::LENGTH_RATIO_C_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(21.406, 24.806)), module, Reverb::LENGTH_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(45.822, 24.806)), module, Reverb::LENGTH_RATIO_B_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(37.854, 34.442)), module, Reverb::LENGTH_RATIO_A_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(19.867, 63.49)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(43.006, 63.49)), module, Reverb::DIFF_MODE_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(32.597, 93.356)), module, Reverb::FEEDBACK_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, Reverb::RIGHT_OUTPUT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("reverb");
