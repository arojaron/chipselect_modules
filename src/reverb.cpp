#include "plugin.hpp"

#include "components/diffusion_stage.hpp"
#include "components/one_pole.hpp"
#include "components/matched_biquad.hpp"
#include "components/matched_shelving.hpp"
#include "components/transient_detection.hpp"

struct Reverb : Module {
	enum ParamId {
		SIZE_PARAM,
		TONE_PARAM,
		SIZE_MOD_PARAM,
		MODEL_SWITCH_PARAM,
		DIFF_PARAM,
		DIFF_MOD_PARAM,
		FEEDBACK_PARAM,
		DUCKING_PARAM,
		FEEDBACK_MOD_PARAM,
		PREDELAY_PARAM,
		DRY_PARAM,
		WET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TONE_MOD_INPUT,
		SIZE_MOD_INPUT,
		DIFF_MOD_INPUT,
		FEEDBACK_MOD_INPUT,
		DRY_MOD_INPUT,
		WET_MOD_INPUT,
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
		MODEL1_LIGHT,
		MODEL2_LIGHT,
		DUCKING_LIGHT,
		LIGHTS_LEN
	};

	struct ReverbParameters{
		simd::float_4 lengths[5] = {};
		simd::float_4 normals[5] = {
			simd::float_4(1, 1, 1, 1),
			simd::float_4(1, 1, 1, 1),
			simd::float_4(1, 1, 1, 1),
			simd::float_4(1, 1, 1, 1),
			simd::float_4(1, 1, 1, 1)
		};
	} rev_params;

	float FS = 48000.0;
	cs::DelayStage4 predelay;
	cs::DiffusionStage diffusion1;
	cs::DiffusionStage diffusion2;
	cs::DiffusionStage diffusion3;
	cs::DiffusionStage diffusion4;
	cs::DiffusionStage delay;
	cs::OnePole<simd::float_4> hp_filter;
	cs::TwoShelves<simd::float_4> two_shelves;
	cs::TransientDetector duck;

	unsigned model_index = 0;
	simd::float_4 back_fed = simd::float_4::zero();

	Reverb() 
	: predelay(cs::DelayStage4(simd::float_4(0.25f), FS)),
	  diffusion1(cs::DiffusionStage(rev_params.lengths[0], rev_params.normals[0], FS)),
	  diffusion2(cs::DiffusionStage(rev_params.lengths[1], rev_params.normals[1], FS)),
	  diffusion3(cs::DiffusionStage(rev_params.lengths[2], rev_params.normals[2], FS)),
	  diffusion4(cs::DiffusionStage(rev_params.lengths[3], rev_params.normals[3], FS)),
	  delay(cs::DiffusionStage(rev_params.lengths[4], rev_params.normals[4], FS)),
	  hp_filter(cs::OnePole<simd::float_4>(FS)),
	  two_shelves(cs::TwoShelves<simd::float_4>(FS)),
	  duck(cs::TransientDetector(FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SIZE_PARAM, 0.f, 1.f, 0.5f, "Size");
		configParam(SIZE_MOD_PARAM, -1.f, 1.f, 0.f, "Size modulation depth");
		configInput(SIZE_MOD_INPUT, "Size modulation");
		configParam(TONE_PARAM, -0.99f, 0.99f, 0.f, "Tone");
		configInput(TONE_MOD_INPUT, "Tone modulation");
		configParam(PREDELAY_PARAM, 0.f, 1.f, 0.f, "Predelay");
		configParam(DIFF_PARAM, 0.f, 0.5f, 0.f, "Diffusion");
		configParam(DIFF_MOD_PARAM, -1.f, 1.f, 0.f, "Diffusion modulation depth");
		configInput(DIFF_MOD_INPUT, "Diffusion modulation");
		configParam(MODEL_SWITCH_PARAM, 0.f, 1.f, 0.f, "Model selector");
		configParam(DRY_PARAM, 0.f, 1.f, 0.5f, "Dry level");
		configParam(WET_PARAM, 0.f, 1.f, 0.5f, "Wet level");
		configInput(DRY_MOD_INPUT, "Dry modulation");
		configInput(WET_MOD_INPUT, "Wet modulation");
		configParam(FEEDBACK_PARAM, std::log2(0.1f), std::log2(200.f), std::log2(5.f), "Reverb time", "s", 2);
		configParam(FEEDBACK_MOD_PARAM, -1.f, 1.f, 0.f, "Reverb time modulation depth");
		configInput(FEEDBACK_MOD_INPUT, "Reverb time modulation");
		configParam(DUCKING_PARAM, 0.f, 1.f, 0.f, "Ducking");
		configInput(LEFT_INPUT, "Left");
		configInput(RIGHT_INPUT, "Right");
		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");

		model_index = loadModel(model_index);
		hp_filter.setFrequency(10.f);
	}

	struct ProcessorParameters{
		float predelay_time;
		float diffusion_depth;
		float delay_scale;
		float high_shelf_gain;
		float low_shelf_gain;
		float const shelving_center = 400.f;
		float feedback;
		float dry;
		float wet;
		float ducking_depth;
	} p;

	void calculateProcessorParameters(simd::float_4 const in_signal)
	{
		p.predelay_time = dsp::cubic(params[PREDELAY_PARAM].getValue());

		p.diffusion_depth = params[DIFF_PARAM].getValue() + dsp::cubic(params[DIFF_MOD_PARAM].getValue()*inputs[DIFF_MOD_INPUT].getVoltage()*0.1f);
		p.diffusion_depth = clamp(p.diffusion_depth, 0.f, 0.5f);
		float delay_rem = 1.f - p.diffusion_depth;
		p.delay_scale = params[SIZE_PARAM].getValue();
		p.delay_scale = p.delay_scale*p.delay_scale;
		float delay_vpoct = dsp::approxExp2_taylor5(params[SIZE_MOD_PARAM].getValue()*inputs[SIZE_MOD_INPUT].getVoltage());
		p.delay_scale /= delay_vpoct;
		float delay_time = p.delay_scale;
		p.diffusion_depth *= p.delay_scale;
		p.delay_scale *= delay_rem;

		float tone = params[TONE_PARAM].getValue() + 0.1*inputs[TONE_MOD_INPUT].getVoltage();
		tone = clamp(tone, -0.99f, 0.99f);
		p.high_shelf_gain = 1.f;
		p.low_shelf_gain = 1.f;
		if(tone < 0){
			p.high_shelf_gain = 1.f - tone*tone;
		}
		else{
			p.low_shelf_gain = 1.f - tone*tone;
		}

		p.dry = params[DRY_PARAM].getValue();
		p.dry += inputs[DRY_MOD_INPUT].getVoltage() * 0.1;
		p.dry = clamp(p.dry);
		p.wet = params[WET_PARAM].getValue();
		p.wet += inputs[WET_MOD_INPUT].getVoltage() * 0.1;
		p.wet = clamp(p.wet);

		float ducking_scale = (dsp::quintic(params[DUCKING_PARAM].getValue()));
		p.ducking_depth = duck.process(ducking_scale*(in_signal[0] + in_signal[1]));

		float reverb_time = dsp::approxExp2_taylor5(params[FEEDBACK_PARAM].getValue() + params[FEEDBACK_MOD_PARAM].getValue()*inputs[FEEDBACK_MOD_INPUT].getVoltage());
		p.feedback = (1.f - p.ducking_depth) * std::exp2(-6.f*(delay_time/reverb_time));
	}

	void process(const ProcessArgs& args) override
	{
		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);

		calculateProcessorParameters(v);

		predelay.setScale(p.predelay_time);
		diffusion1.setScale(p.diffusion_depth);
		diffusion2.setScale(p.diffusion_depth);
		diffusion3.setScale(p.diffusion_depth);
		diffusion4.setScale(p.diffusion_depth);
		delay.setScale(p.delay_scale);
		two_shelves.setParams(p.shelving_center, p.low_shelf_gain, p.high_shelf_gain);

		lights[DUCKING_LIGHT].setBrightnessSmooth(p.ducking_depth, args.sampleTime);

		v = predelay.process(v);
		v = v + back_fed;
		v = v - hp_filter.process(v);
		v = two_shelves.process(v);
		v = diffusion1.process(v);
		v = diffusion2.process(v);
		v = diffusion3.process(v);
		v = diffusion4.process(v);
		left = p.wet*v[0] + p.dry*left;
		right = p.wet*v[1] + p.dry*right;
		v = delay.process(v);
		back_fed = v * simd::float_4(p.feedback);

		outputs[LEFT_OUTPUT].setVoltage(left);
		outputs[RIGHT_OUTPUT].setVoltage(right);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		reloadProcessors();
	}

	void loadNextModel(void)
	{
		model_index = loadModel(++model_index);
	}

	unsigned loadReverbParameters(ReverbParameters& params, unsigned model_index);
	unsigned loadModel(unsigned index)
	{
		unsigned ret = loadReverbParameters(rev_params, model_index);
		reloadProcessors();
		lights[MODEL1_LIGHT].setBrightness(0);
		lights[MODEL2_LIGHT].setBrightness(0);
		switch(ret){
		case 0:
			lights[MODEL1_LIGHT].setBrightness(1);
			break;
		case 1:
			lights[MODEL2_LIGHT].setBrightness(1);
			break;
		default:
			break;
		};
		return ret;
	}

	void reloadProcessors(void)
	{
		back_fed = simd::float_4::zero();
		predelay = cs::DelayStage4(simd::float_4(0.25f), FS);
		diffusion1 = cs::DiffusionStage(rev_params.lengths[0], rev_params.normals[0], FS);
		diffusion2 = cs::DiffusionStage(rev_params.lengths[1], rev_params.normals[1], FS);
		diffusion3 = cs::DiffusionStage(rev_params.lengths[2], rev_params.normals[2], FS);
		diffusion4 = cs::DiffusionStage(rev_params.lengths[3], rev_params.normals[3], FS);
	  	delay = cs::DiffusionStage(rev_params.lengths[4], rev_params.normals[4], FS);
		hp_filter = cs::OnePole<simd::float_4>(FS);
		hp_filter.setFrequency(10.f);
		two_shelves = cs::TwoShelves<simd::float_4>(FS);
		duck = cs::TransientDetector(FS);
	}

	json_t* dataToJson() override
	{
		json_t* root = json_object();
		json_object_set_new(root, "model", json_integer(model_index));
		return root;
	}

	void dataFromJson(json_t* root) override
	{
		json_t* model_json = json_object_get(root, "model");
		if(model_json){
			model_index = json_integer_value(model_json);
			model_index = loadModel(model_index);
		}
	}
};

struct DiffModeButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		VCVButton::onDragStart(e);
		((Reverb*)this->module)->loadNextModel();
	}
};

struct ReverbWidget : ModuleWidget {
	ReverbWidget(Reverb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.243, 19.05)), module, Reverb::SIZE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.8, 22.852)), module, Reverb::TONE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.479, 24.118)), module, Reverb::SIZE_MOD_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(48.259, 43.18)), module, Reverb::MODEL_SWITCH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.243, 45.726)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.477, 50.792)), module, Reverb::DIFF_MOD_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(38.099, 67.31)), module, Reverb::FEEDBACK_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.779, 71.15)), module, Reverb::DUCKING_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(53.337, 72.391)), module, Reverb::FEEDBACK_MOD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.239, 95.25)), module, Reverb::PREDELAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(33.019, 95.25)), module, Reverb::DRY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.799, 95.258)), module, Reverb::WET_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.419, 15.235)), module, Reverb::TONE_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.479, 16.506)), module, Reverb::SIZE_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.477, 43.18)), module, Reverb::DIFF_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(53.337, 64.779)), module, Reverb::FEEDBACK_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.639, 87.643)), module, Reverb::DRY_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.419, 87.643)), module, Reverb::WET_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.698, 110.484)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.857, 110.484)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.179, 110.486)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.338, 110.486)), module, Reverb::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(55.748, 44.59)), module, Reverb::MODEL1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(60.863, 44.59)), module, Reverb::MODEL2_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(25.395, 64.779)), module, Reverb::DUCKING_LIGHT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("Reverb");

unsigned Reverb::loadReverbParameters(ReverbParameters& params, unsigned model_index)
{
	std::string params_filename = asset::plugin(pluginInstance, "src/reverb_constants.json");
	
	FILE* file = fopen(params_filename.c_str(), "r");
	json_error_t err;
	json_t* file_j = json_loadf(file, 0, &err);
	size_t models_length = json_array_size(file_j);
	if(models_length){
		model_index %= models_length;
		json_t* model_j = json_array_get(file_j, model_index);

		json_t* lengths_j = json_object_get(model_j, "lengths");
		for(int i = 0; i < 5; i++){
			json_t* lengths_i_j = json_array_get(lengths_j, i);
			params.lengths[i] = simd::float_4(
			json_real_value(json_array_get(lengths_i_j, 0)), 
			json_real_value(json_array_get(lengths_i_j, 1)), 
			json_real_value(json_array_get(lengths_i_j, 2)), 
			json_real_value(json_array_get(lengths_i_j, 3)));
			json_decref(lengths_i_j);
		}
		json_decref(lengths_j);

		json_t* normals_j = json_object_get(model_j, "mixer_normals");
		for(int i = 0; i < 5; i++){
			json_t* normals_i_j = json_array_get(normals_j, i);
			params.normals[i] = simd::float_4(
			json_real_value(json_array_get(normals_i_j, 0)), 
			json_real_value(json_array_get(normals_i_j, 1)), 
			json_real_value(json_array_get(normals_i_j, 2)), 
			json_real_value(json_array_get(normals_i_j, 3)));
			json_decref(normals_i_j);
		}
		json_decref(normals_j);
		json_decref(lengths_j);
		json_decref(model_j);
		json_decref(file_j);
		return model_index;
	}
	json_decref(file_j);
	return 0;
}