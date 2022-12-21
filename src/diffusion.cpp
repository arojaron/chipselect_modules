#include "plugin.hpp"
#include "diffusion_stage.hpp"
#include "reverb_constants.hpp"

struct Diffusion : Module {
	enum ParamId {
		DIFF_PARAM,
		DIFFMODSCALE_PARAM,
		MODE_SELECT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DIFFMODSIGNAL_INPUT,
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
		MODE_1_LIGHT,
		MODE_2_LIGHT,
		MODE_3_LIGHT,
		MODE_4_LIGHT,
		LIGHTS_LEN
	};

	unsigned mode = 0;
	float FS = 48000.0;
	DiffusionStage4 diff_stage;

	Diffusion()
	: diff_stage(DiffusionStage4(lengths_A, mix_coefs_A, FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DIFF_PARAM, 0.f, 1.f, 0.f, "Diffusion depth");
		configParam(DIFFMODSCALE_PARAM, -1.f, 1.f, 0.f, "Modulation depth");
		configButton(MODE_SELECT_PARAM, "Mode selector");
		configInput(DIFFMODSIGNAL_INPUT, "Modulation");
		configInput(LEFT_INPUT, "Left");
		configInput(RIGHT_INPUT, "Right");
		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");
	}

	void process(const ProcessArgs& args) override 
	{			
		float diff_depth = params[DIFF_PARAM].getValue();
		float diff_mod_scale = params[DIFFMODSCALE_PARAM].getValue();
		float diff_mod = inputs[DIFFMODSIGNAL_INPUT].getVoltage();
		diff_depth = dsp::cubic(diff_depth + 0.1*diff_mod*diff_mod_scale);
		diff_stage.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;

		simd::float_4 v = simd::float_4(left, right, left, right);
		v = diff_stage.process(v);

		outputs[LEFT_OUTPUT].setVoltage(v[0] + v[2]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1] + v[3]);
	}

	void modeSelect(void)
	{
		switch(mode){
		case 0:
			diff_stage = DiffusionStage4(lengths_A, mix_coefs_A, FS);
			lights[MODE_1_LIGHT].setBrightness(0.7);
			lights[MODE_4_LIGHT].setBrightness(0.0);
			break;
		case 1:
			diff_stage = DiffusionStage4(lengths_B, mix_coefs_B, FS);
			lights[MODE_2_LIGHT].setBrightness(0.7);
			lights[MODE_1_LIGHT].setBrightness(0.0);
			break;
		case 2:
			diff_stage = DiffusionStage4(lengths_C, mix_coefs_A, FS);
			lights[MODE_3_LIGHT].setBrightness(0.7);
			lights[MODE_2_LIGHT].setBrightness(0.0);
			break;
		case 3:
			diff_stage = DiffusionStage4(lengths_D, mix_coefs_A, FS);
			lights[MODE_4_LIGHT].setBrightness(0.7);
			lights[MODE_3_LIGHT].setBrightness(0.0);
			break;
		default:
			break;
		};
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		modeSelect();
	}
	void onModeChange(void)
	{
		mode++;
		if(mode >= 4) mode = 0;
		modeSelect();
	}
};

struct ModeButton : VCVButton {
	void onDragStart(const DragStartEvent& e) override
	{
		((Diffusion*)this->module)->onModeChange();
		VCVButton::onDragStart(e);
	}
};

struct DiffusionWidget : ModuleWidget {
	DiffusionWidget(Diffusion* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/diffusion.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 18.504)), module, Diffusion::DIFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 34.448)), module, Diffusion::DIFFMODSCALE_PARAM));
		addParam(createParamCentered<ModeButton>(mm2px(Vec(11.514, 72.418)), module, Diffusion::MODE_SELECT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 44.665)), module, Diffusion::DIFFMODSIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.971, 104.53)), module, Diffusion::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.971, 114.491)), module, Diffusion::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.509, 104.526)), module, Diffusion::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.509, 114.457)), module, Diffusion::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(19.966, 65.11)), module, Diffusion::MODE_1_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(19.966, 69.982)), module, Diffusion::MODE_2_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(19.966, 74.854)), module, Diffusion::MODE_3_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(19.966, 79.726)), module, Diffusion::MODE_4_LIGHT));
	}
};

Model* modelDiffusion = createModel<Diffusion, DiffusionWidget>("diffusion");
