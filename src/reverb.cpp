#include "plugin.hpp"

#include "components/diffusion_stage.hpp"
#include "components/one_pole.hpp"
#include "components/matched_biquad.hpp"
#include "components/transient_detection.hpp"

extern simd::float_4 diff_lengths[];
extern simd::float_4 mixer_normals[];

struct Reverb : Module {
	enum ParamId {
		LENGTH_RATIO_A_PARAM,
		LENGTH_PARAM,
		LENGTH_RATIO_B_PARAM,
		LENGTH_RATIO_C_PARAM,
		HP_PARAM,
		LP_PARAM,
		DIFF_PARAM,
		DIFF_MODE_PARAM,
		DRYWET_PARAM,
		FEEDBACK_PARAM,
		DUCKING_PARAM,
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
		DUCKING_LIGHT,
		LIGHTS_LEN
	};

	float FS = 48000.0;
	cs::DiffusionStage diffusion1;
	cs::DiffusionStage diffusion2;
	cs::DiffusionStage diffusion3;
	cs::DiffusionStage diffusion4;
	cs::DiffusionStage delay;
	cs::OnePole<simd::float_4> hp_filter;
	cs::OnePole<simd::float_4> lp_filter;

	cs::Ducking duck;

	Reverb() 
	: diffusion1(cs::DiffusionStage(diff_lengths[0], mixer_normals[0], FS)),
	  diffusion2(cs::DiffusionStage(diff_lengths[1], mixer_normals[1], FS)),
	  diffusion3(cs::DiffusionStage(diff_lengths[2], mixer_normals[2], FS)),
	  diffusion4(cs::DiffusionStage(diff_lengths[3], mixer_normals[3], FS)),
	  delay(cs::DiffusionStage(simd::float_4(1,1,1,1), mixer_normals[1], FS)),
	  hp_filter(cs::OnePole<simd::float_4>(FS)),
	  lp_filter(cs::OnePole<simd::float_4>(FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LENGTH_RATIO_C_PARAM, 0.f, 1.f, 0.f, "Length ratio C");
		configParam(LENGTH_PARAM, 0.f, 1.f, 0.f, "Length main", "s");
		configParam(LENGTH_RATIO_B_PARAM, 0.f, 1.f, 0.f, "Length ratio B");
		configParam(LENGTH_RATIO_A_PARAM, 0.f, 1.f, 0.f, "Length ratio A");
		configParam(HP_PARAM, 0.f, 1.f, 0.f, "High pass");
		configParam(LP_PARAM, 0.f, 1.f, 1.f, "Low pass");
		configParam(DIFF_PARAM, 0.f, 1.f, 0.f, "Diffusion");
		configParam(DIFF_MODE_PARAM, 0.f, 1.f, 0.f, "Diffusion mode");
		configParam(DRYWET_PARAM, 0.f, 1.f, 1.f, "Dry-Wet");
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Feedback");
		configParam(DUCKING_PARAM, 0.f, 1.f, 0.f, "Ducking");
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
		diffusion3.setScale(diff_depth);
		diffusion4.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;

		duck.setScaling(dsp::cubic(params[DUCKING_PARAM].getValue()));
		float ducking_depth = duck.process(0.5*(left + right));
		lights[DUCKING_LIGHT].setBrightness(ducking_depth);

		simd::float_4 v = simd::float_4(left, right, left, right);

		float hp_param = dsp::quintic(params[HP_PARAM].getValue());
		float lp_param = dsp::quintic(params[LP_PARAM].getValue());
		hp_filter.setFrequency(0.5*FS*hp_param);
		lp_filter.setFrequency(0.5*FS*lp_param);
		
		v = v-hp_filter.process(v);
		v = v + back_fed;
		v = lp_filter.process(v);

		v = diffusion1.process(v);
		v = diffusion2.process(v);
		v = diffusion3.process(v);
		v = diffusion4.process(v);

		float drywet = params[DRYWET_PARAM].getValue();
		outputs[LEFT_OUTPUT].setVoltage(drywet*v[0] + (1-drywet)*left);
		outputs[RIGHT_OUTPUT].setVoltage(drywet*v[1] + (1-drywet)*right);

		float delay0 = params[LENGTH_PARAM].getValue();
		float delayA = delay0 * params[LENGTH_RATIO_A_PARAM].getValue();
		float delayB = delay0 * params[LENGTH_RATIO_B_PARAM].getValue();
		float delayC = delay0 * params[LENGTH_RATIO_C_PARAM].getValue();
		simd::float_4 delay_param = simd::float_4(delayA, delayB, delayC, delay0);
		delay_param = dsp::cubic(delay_param);
		delay.setScale(delay_param);

		v = delay.process(v);

		float feedback = params[FEEDBACK_PARAM].getValue();
		feedback = (1-ducking_depth)*feedback;
		back_fed = v * simd::float_4(feedback);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		diffusion1 = cs::DiffusionStage(diff_lengths[0], mixer_normals[0], FS);
		diffusion2 = cs::DiffusionStage(diff_lengths[1], mixer_normals[1], FS);
		diffusion3 = cs::DiffusionStage(diff_lengths[2], mixer_normals[2], FS);
		diffusion4 = cs::DiffusionStage(diff_lengths[3], mixer_normals[3], FS);
	  	delay = cs::DiffusionStage(simd::float_4(1,1,1,1), mixer_normals[1], FS);
		hp_filter = cs::OnePole<simd::float_4>(FS);
		lp_filter = cs::OnePole<simd::float_4>(FS);
	}
};

struct DiffModeButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		VCVButton::onDragStart(e);
	}
};

struct ReverbWidget : ModuleWidget {
	ReverbWidget(Reverb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 9.439)), module, Reverb::LENGTH_RATIO_A_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(21.406, 19.074)), module, Reverb::LENGTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.822, 19.074)), module, Reverb::LENGTH_RATIO_B_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 28.709)), module, Reverb::LENGTH_RATIO_C_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.435, 42.996)), module, Reverb::HP_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.525, 42.996)), module, Reverb::LP_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(16.65, 63.551)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(39.789, 63.551)), module, Reverb::DIFF_MODE_PARAM));
		addParam(createParamCentered<Davies1900hRedKnob>(mm2px(Vec(14.023, 86.519)), module, Reverb::DRYWET_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(45.145, 93.386)), module, Reverb::FEEDBACK_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.89, 98.739)), module, Reverb::DUCKING_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, Reverb::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(31.423, 90.619)), module, Reverb::DUCKING_LIGHT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("Reverb");