#include "plugin.hpp"

#include "components/diffusion_stage.hpp"
#include "components/one_pole.hpp"
#include "components/matched_biquad.hpp"
#include "components/matched_shelving.hpp"
#include "components/transient_detection.hpp"

struct Reverb : Module {
	enum ParamId {
		LENGTH_PARAM,
		LENGTH_MOD_PARAM,
		TONE_HIGH_PARAM,
		TONE_CENTER_PARAM,
		TONE_LOW_PARAM,
		DIFF_PARAM,
		DIFF_MOD_PARAM,
		DIFF_MODE_PARAM,
		DRYWET_PARAM,
		FEEDBACK_PARAM,
		DUCKING_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LENGTH_MOD_INPUT,
		DIFF_MOD_INPUT,
		DRYWET_MOD_INPUT,
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
	};

	unsigned loadReverbParameters(ReverbParameters& params, unsigned model_index);

	ReverbParameters rev_params;

	float FS = 48000.0;
	cs::DiffusionStage diffusion1;
	cs::DiffusionStage diffusion2;
	cs::DiffusionStage diffusion3;
	cs::DiffusionStage diffusion4;
	cs::DiffusionStage delay;
	cs::OnePole<simd::float_4> hp_filter;
	cs::TransientDetector duck;

	cs::TwoShelves<simd::float_4> two_shelves;

	unsigned model_index = 0;
	simd::float_4 back_fed = simd::float_4::zero();

	Reverb() 
	: diffusion1(cs::DiffusionStage(rev_params.lengths[0], rev_params.normals[0], FS)),
	  diffusion2(cs::DiffusionStage(rev_params.lengths[1], rev_params.normals[1], FS)),
	  diffusion3(cs::DiffusionStage(rev_params.lengths[2], rev_params.normals[2], FS)),
	  diffusion4(cs::DiffusionStage(rev_params.lengths[3], rev_params.normals[3], FS)),
	  delay(cs::DiffusionStage(rev_params.lengths[4], rev_params.normals[4], FS)),
	  hp_filter(cs::OnePole<simd::float_4>(FS)),
	  duck(cs::TransientDetector(FS)),
	  two_shelves(cs::TwoShelves<simd::float_4>(FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LENGTH_PARAM, 0.f, 1.f, 0.5f, "Size");
		configParam(LENGTH_MOD_PARAM, -1.f, 1.f, 0.f, "Size modulation depth");
		configInput(LENGTH_MOD_INPUT, "Size modulation");
		configParam(TONE_HIGH_PARAM, -0.99f, 0.99f, 0.f, "Tone");
		configParam(TONE_CENTER_PARAM, std::log2(500.f), std::log2(10000.f), std::log2(500.f), "Tone center", "Hz", 2);
		configParam(TONE_LOW_PARAM, 0.1f, 1.f, 0.9f, "Low shelf");
		configParam(DIFF_PARAM, 0.f, 1.f, 0.f, "Diffusion");
		configParam(DIFF_MOD_PARAM, -1.f, 1.f, 0.f, "Diffusion modulation depth");
		configInput(DIFF_MOD_INPUT, "Diffusion modulation");
		configParam(DIFF_MODE_PARAM, 0.f, 1.f, 0.f, "Diffusion mode");
		configParam(DRYWET_PARAM, 0.f, 1.f, 0.5f, "Dry-Wet");
		configParam(FEEDBACK_PARAM, std::log2(0.1f), std::log2(200.f), std::log2(0.1f), "Reverb time", "s", 2);
		configParam(DUCKING_PARAM, 0.f, 1.f, 0.f, "Ducking");
		configInput(LEFT_INPUT, "Left");
		configInput(RIGHT_INPUT, "Right");
		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");

		model_index = loadModel(model_index);
	}

	void process(const ProcessArgs& args) override
	{
		// reading inputs
		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;

		// setting parameters
		float diff_depth = params[DIFF_PARAM].getValue();
		float delay_rem = 1.f - diff_depth;
		float delay_scale = params[LENGTH_PARAM].getValue();
		delay_scale = delay_scale*delay_scale;
		float delay_vpoct = dsp::approxExp2_taylor5(params[LENGTH_MOD_PARAM].getValue()*inputs[LENGTH_MOD_INPUT].getVoltage());
		delay_scale /= delay_vpoct;

		float delay_time = delay_scale;
		diff_depth *= delay_scale;
		delay_scale *= delay_rem;

		diffusion1.setScale(diff_depth);
		diffusion2.setScale(diff_depth);
		diffusion3.setScale(diff_depth);
		diffusion4.setScale(diff_depth);
		delay.setScale(delay_scale);

		hp_filter.setFrequency(10.f);

		float tone = params[TONE_HIGH_PARAM].getValue();
		float high_shelf = 1.f;
		float low_shelf = 1.f;
		if(tone < 0){
			high_shelf = 1.f - tone*tone;
		}
		else{
			low_shelf = 1.f - tone*tone;
		}
		two_shelves.setParams(400.f, low_shelf, high_shelf);
		
		float drywet = params[DRYWET_PARAM].getValue();
		drywet += inputs[DRYWET_MOD_INPUT].getVoltage() * 0.1;
		drywet = clamp(drywet);

		float duck_scaling = (dsp::cubic(params[DUCKING_PARAM].getValue()));
		float ducking_depth = duck.process(duck_scaling*(left + right));
		lights[DUCKING_LIGHT].setBrightnessSmooth(ducking_depth, args.sampleTime);

		float reverb_time = dsp::approxExp2_taylor5(params[FEEDBACK_PARAM].getValue());
		// float rt_2mag = -6*3.32192809489*(delay_time/reverb_time);	// *log2(10)
		float rt_2mag = -6*(delay_time/reverb_time);
		float feedback = (1-ducking_depth)*dsp::approxExp2_taylor5(rt_2mag);
		//simd::float_4 actual_high_gain = two_shelves.getActualHighGain();
		//feedback /= actual_high_gain[0];

		// processing signal
		simd::float_4 v = simd::float_4(left, right, left, right);
		
		v = v + back_fed;
		v = v - hp_filter.process(v);

		v = two_shelves.process(v);

		v = diffusion1.process(v);
		v = diffusion2.process(v);
		v = diffusion3.process(v);
		v = diffusion4.process(v);

		left = drywet*v[0] + (1-drywet)*left;
		right = drywet*v[1] + (1-drywet)*right;

		v = delay.process(v);
		back_fed = v * simd::float_4(feedback);

		// writing outputs
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

	unsigned loadModel(unsigned index)
	{
		unsigned ret = loadReverbParameters(rev_params, model_index);
		reloadProcessors();
		lights[MODE_1_LIGHT].setBrightness(0);
		lights[MODE_2_LIGHT].setBrightness(0);
		lights[MODE_3_LIGHT].setBrightness(0);
		lights[MODE_4_LIGHT].setBrightness(0);
		switch(ret){
		case 0:
			lights[MODE_1_LIGHT].setBrightness(1);
			break;
		case 1:
			lights[MODE_2_LIGHT].setBrightness(1);
			break;
		case 2:
			lights[MODE_3_LIGHT].setBrightness(1);
			break;
		case 3:
			lights[MODE_4_LIGHT].setBrightness(1);
			break;
		default:
			break;
		};
		return ret;
	}

	void reloadProcessors(void)
	{
		back_fed = simd::float_4::zero();
		diffusion1 = cs::DiffusionStage(rev_params.lengths[0], rev_params.normals[0], FS);
		diffusion2 = cs::DiffusionStage(rev_params.lengths[1], rev_params.normals[1], FS);
		diffusion3 = cs::DiffusionStage(rev_params.lengths[2], rev_params.normals[2], FS);
		diffusion4 = cs::DiffusionStage(rev_params.lengths[3], rev_params.normals[3], FS);
	  	delay = cs::DiffusionStage(rev_params.lengths[4], rev_params.normals[4], FS);
		hp_filter = cs::OnePole<simd::float_4>(FS);
		two_shelves = cs::TwoShelves<simd::float_4>(FS);
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

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(23.523, 20.926)), module, Reverb::LENGTH_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(41.495, 23.148)), module, Reverb::LENGTH_MOD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.902, 38.234)), module, Reverb::TONE_HIGH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.899, 39.85)), module, Reverb::TONE_CENTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.248, 48.552)), module, Reverb::TONE_LOW_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(19.296, 50.315)), module, Reverb::DIFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(13.547, 68.21)), module, Reverb::DIFF_MOD_PARAM));
		addParam(createParamCentered<DiffModeButton>(mm2px(Vec(44.332, 69.447)), module, Reverb::DIFF_MODE_PARAM));
		addParam(createParamCentered<Davies1900hRedKnob>(mm2px(Vec(14.287, 88.9)), module, Reverb::DRYWET_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(45.145, 96.547)), module, Reverb::FEEDBACK_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.89, 101.899)), module, Reverb::DUCKING_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.759, 12.5)), module, Reverb::LENGTH_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.718, 68.41)), module, Reverb::DIFF_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.89, 83.143)), module, Reverb::DRYWET_MOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, Reverb::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, Reverb::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, Reverb::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, Reverb::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.354, 60.745)), module, Reverb::MODE_1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.049, 67.727)), module, Reverb::MODE_2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.545, 74.583)), module, Reverb::MODE_3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(49.212, 78.131)), module, Reverb::MODE_4_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(31.423, 93.78)), module, Reverb::DUCKING_LIGHT));
	}
};


Model* modelReverb = createModel<Reverb, ReverbWidget>("Reverb");

unsigned Reverb::loadReverbParameters(ReverbParameters& params, unsigned model_index)
{
	std::string params_filename = asset::user("chipselect_reverb_constants.json");
	
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