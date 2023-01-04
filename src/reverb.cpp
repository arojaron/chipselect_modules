#include "plugin.hpp"

#include "components/diffusion_stage.hpp"
#include "components/one_pole.hpp"
#include "components/matched_biquad.hpp"

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
	DiffusionStage diffusion1;
	DiffusionStage diffusion2;
	DiffusionStage diffusion3;
	DiffusionStage diffusion4;
	DiffusionStage delay;
	cs::OnePole<simd::float_4> hp_filter;
	cs::OnePole<simd::float_4> lp_filter;

	Reverb() 
	: diffusion1(DiffusionStage(diff_lengths[0], mixer_normals[0], FS)),
	  diffusion2(DiffusionStage(diff_lengths[1], mixer_normals[1], FS)),
	  diffusion3(DiffusionStage(diff_lengths[2], mixer_normals[2], FS)),
	  diffusion4(DiffusionStage(diff_lengths[3], mixer_normals[3], FS)),
	  delay(DiffusionStage(simd::float_4(1,1,1,1), mixer_normals[1], FS)),
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
		diffusion3.setScale(diff_depth);
		diffusion4.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);

		v = v + back_fed;

		float hp_param = params[HP_PARAM].getValue();
		hp_param *= hp_param*hp_param*hp_param*hp_param;
		hp_filter.setFrequency(0.5*FS*hp_param);
		float lp_param = params[LP_PARAM].getValue();
		lp_param *= lp_param*lp_param*lp_param*lp_param;
		lp_filter.setFrequency(0.5*FS*lp_param);
		v = v-hp_filter.process(v);
		v = lp_filter.process(v);

		v = diffusion1.process(v);
		v = diffusion2.process(v);
		v = diffusion3.process(v);
		v = diffusion4.process(v);

		outputs[LEFT_OUTPUT].setVoltage(v[0]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1]);

		float delay0 = params[LENGTH_PARAM].getValue();
		float delayA = delay0 * params[LENGTH_RATIO_A_PARAM].getValue();
		float delayB = delay0 * params[LENGTH_RATIO_B_PARAM].getValue();
		float delayC = delay0 * params[LENGTH_RATIO_C_PARAM].getValue();
		simd::float_4 delay_param = simd::float_4(delayA, delayB, delayC, delay0);
		delay_param = dsp::cubic(delay_param);
		delay.setScale(delay_param);

		v = delay.process(v);

		float feedback = params[FEEDBACK_PARAM].getValue();
		back_fed = v * simd::float_4(feedback);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		diffusion1 = DiffusionStage(diff_lengths[0], mixer_normals[0], FS);
		diffusion2 = DiffusionStage(diff_lengths[1], mixer_normals[1], FS);
		diffusion3 = DiffusionStage(diff_lengths[2], mixer_normals[2], FS);
		diffusion4 = DiffusionStage(diff_lengths[3], mixer_normals[3], FS);
	  	delay = DiffusionStage(simd::float_4(1,1,1,1), mixer_normals[1], FS);
		hp_filter = cs::OnePole<simd::float_4>(FS);
		lp_filter = cs::OnePole<simd::float_4>(FS);
	}
};

struct DiffModeButton : VCVButton{

};

struct ReverbWidget : ModuleWidget {
	ReverbWidget(Reverb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 12.552)), module, Reverb::LENGTH_RATIO_A_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(21.406, 22.187)), module, Reverb::LENGTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.822, 22.187)), module, Reverb::LENGTH_RATIO_B_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.854, 31.822)), module, Reverb::LENGTH_RATIO_C_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.435, 50.14)), module, Reverb::HP_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.525, 50.14)), module, Reverb::LP_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(20.09, 72.486)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(43.229, 72.486)), module, Reverb::DIFF_MODE_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(30.48, 93.356)), module, Reverb::FEEDBACK_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, Reverb::RIGHT_OUTPUT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("reverb");