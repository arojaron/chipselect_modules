#include "plugin.hpp"

#include "components/diffusion_stage.hpp"

static simd::float_4 const init_lengths = simd::float_4::zero();
static simd::float_4 const init_normal = simd::float_4(1, 1, 1, 1);

struct ReverbDesigner : Module {
	enum ParamId {
		DEL11_PARAM,
		DEL21_PARAM,
		DEL31_PARAM,
		DEL41_PARAM,
		DEL51_PARAM,
		DEL12_PARAM,
		DEL22_PARAM,
		DEL32_PARAM,
		DEL42_PARAM,
		DEL52_PARAM,
		DEL13_PARAM,
		DEL23_PARAM,
		DEL33_PARAM,
		DEL43_PARAM,
		DEL53_PARAM,
		DEL14_PARAM,
		DEL24_PARAM,
		DEL34_PARAM,
		DEL44_PARAM,
		DEL54_PARAM,
		SCALE1_PARAM,
		SCALE2_PARAM,
		SCALE3_PARAM,
		SCALE4_PARAM,
		SCALE5_PARAM,
		MIX11_PARAM,
		MIX21_PARAM,
		MIX31_PARAM,
		MIX41_PARAM,
		MIX51_PARAM,
		MIX12_PARAM,
		MIX22_PARAM,
		MIX32_PARAM,
		MIX42_PARAM,
		MIX52_PARAM,
		MIX13_PARAM,
		MIX23_PARAM,
		MIX33_PARAM,
		MIX43_PARAM,
		MIX53_PARAM,
		MIX14_PARAM,
		MIX24_PARAM,
		MIX34_PARAM,
		MIX44_PARAM,
		MIX54_PARAM,
		SWITCH1_PARAM,
		SWITCH2_PARAM,
		SWITCH3_PARAM,
		SWITCH4_PARAM,
		SWITCH5_PARAM,
		SCALEA_PARAM,
		SCALEB_PARAM,
		FEEDBACK_PARAM,
		GENERATE_PARAM,
		STORE_PARAM,
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

	simd::float_4 lengths[5] = {init_lengths};
	simd::float_4 normals[5] = {init_normal};

	float FS = 48000.0;
	cs::DiffusionStage stage1;
	cs::DiffusionStage stage2;
	cs::DiffusionStage stage3;
	cs::DiffusionStage stage4;
	cs::DiffusionStage stage5;
	int parts[5] = {};

	ReverbDesigner() 
	: stage1(cs::DiffusionStage(init_lengths, init_normal, FS)),
	  stage2(cs::DiffusionStage(init_lengths, init_normal, FS)),
	  stage3(cs::DiffusionStage(init_lengths, init_normal, FS)),
	  stage4(cs::DiffusionStage(init_lengths, init_normal, FS)),
	  stage5(cs::DiffusionStage(init_lengths, init_normal, FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DEL11_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL21_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL31_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL41_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL51_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL12_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL22_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL32_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL42_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL52_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL13_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL23_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL33_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL43_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL53_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL14_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL24_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL34_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL44_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL54_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SCALE1_PARAM, 0.f, 1.f, 0.125f, "");
		configParam(SCALE2_PARAM, 0.f, 1.f, 0.25f, "");
		configParam(SCALE3_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(SCALE4_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(SCALE5_PARAM, 0.f, 1.f, 1.f, "");
		configParam(MIX11_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX21_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX31_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX41_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX51_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX12_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX22_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX32_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX42_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX52_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX13_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX23_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX33_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX43_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX53_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX14_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX24_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX34_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX44_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX54_PARAM, -1.f, 1.f, 1.f, "");
		configParam(SWITCH1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SWITCH2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SWITCH3_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SWITCH4_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SWITCH5_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SCALEA_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(SCALEB_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(GENERATE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(STORE_PARAM, 0.f, 1.f, 0.f, "");
		configInput(LEFT_INPUT, "");
		configInput(RIGHT_INPUT, "");
		configOutput(LEFT_OUTPUT, "");
		configOutput(RIGHT_OUTPUT, "");
	}
	
	void onSampleRateChange(const SampleRateChangeEvent& e) override
	{
		FS = e.sampleRate;
		genReverb();
	}

	void genReverb(void)
	{
		lengths[0] = simd::float_4(params[DEL11_PARAM].getValue(), params[DEL12_PARAM].getValue(), params[DEL13_PARAM].getValue(), params[DEL14_PARAM].getValue());
		lengths[1] = simd::float_4(params[DEL21_PARAM].getValue(), params[DEL22_PARAM].getValue(), params[DEL23_PARAM].getValue(), params[DEL24_PARAM].getValue());
		lengths[2] = simd::float_4(params[DEL31_PARAM].getValue(), params[DEL32_PARAM].getValue(), params[DEL33_PARAM].getValue(), params[DEL34_PARAM].getValue());
		lengths[3] = simd::float_4(params[DEL41_PARAM].getValue(), params[DEL42_PARAM].getValue(), params[DEL43_PARAM].getValue(), params[DEL44_PARAM].getValue());
		lengths[4] = simd::float_4(params[DEL51_PARAM].getValue(), params[DEL52_PARAM].getValue(), params[DEL53_PARAM].getValue(), params[DEL54_PARAM].getValue());
		lengths[0] *= simd::float_4(params[SCALE1_PARAM].getValue());
		lengths[1] *= simd::float_4(params[SCALE2_PARAM].getValue());
		lengths[2] *= simd::float_4(params[SCALE3_PARAM].getValue());
		lengths[3] *= simd::float_4(params[SCALE4_PARAM].getValue());
		lengths[4] *= simd::float_4(params[SCALE5_PARAM].getValue());
		
		normals[0] = simd::float_4(params[MIX11_PARAM].getValue(), params[MIX12_PARAM].getValue(), params[MIX13_PARAM].getValue(), params[MIX14_PARAM].getValue());
		normals[1] = simd::float_4(params[MIX21_PARAM].getValue(), params[MIX22_PARAM].getValue(), params[MIX23_PARAM].getValue(), params[MIX24_PARAM].getValue());
		normals[2] = simd::float_4(params[MIX31_PARAM].getValue(), params[MIX32_PARAM].getValue(), params[MIX33_PARAM].getValue(), params[MIX34_PARAM].getValue());
		normals[3] = simd::float_4(params[MIX41_PARAM].getValue(), params[MIX42_PARAM].getValue(), params[MIX43_PARAM].getValue(), params[MIX44_PARAM].getValue());
		normals[4] = simd::float_4(params[MIX51_PARAM].getValue(), params[MIX52_PARAM].getValue(), params[MIX53_PARAM].getValue(), params[MIX54_PARAM].getValue());
		stage1 = cs::DiffusionStage(lengths[0], normals[0], FS);
		stage2 = cs::DiffusionStage(lengths[1], normals[1], FS);
		stage3 = cs::DiffusionStage(lengths[2], normals[2], FS);
		stage4 = cs::DiffusionStage(lengths[3], normals[3], FS);
		stage5 = cs::DiffusionStage(lengths[4], normals[4], FS);
	}

	void storeReverb(void)
	{
		json_t* lengths_arr_j = json_array();
		for(int i = 0; i < 5; i++){
			json_t* lengths_i_arr_j = json_array();
			json_array_append_new(lengths_i_arr_j, json_real(lengths[i][0]));
			json_array_append_new(lengths_i_arr_j, json_real(lengths[i][1]));
			json_array_append_new(lengths_i_arr_j, json_real(lengths[i][2]));
			json_array_append_new(lengths_i_arr_j, json_real(lengths[i][3]));
			json_array_append(lengths_arr_j, lengths_i_arr_j);
			json_decref(lengths_i_arr_j);
		}

		json_t* mixer_arr_j = json_array();
		for(int i = 0; i < 5; i++){
			json_t* mixer_i_arr_j = json_array();
			json_array_append_new(mixer_i_arr_j, json_real(normals[i][0]));
			json_array_append_new(mixer_i_arr_j, json_real(normals[i][1]));
			json_array_append_new(mixer_i_arr_j, json_real(normals[i][2]));
			json_array_append_new(mixer_i_arr_j, json_real(normals[i][3]));
			json_array_append(mixer_arr_j, mixer_i_arr_j);
			json_decref(mixer_i_arr_j);
		}

		json_t* params_j = json_object();
		json_object_set(params_j, "lengths", lengths_arr_j);
		json_object_set(params_j, "mixer_normals", mixer_arr_j);

		std::string params_filename = asset::user("chipselect_reverb_constants.json");
		FILE* file = fopen(params_filename.c_str(), "r");
		if(file){
			json_error_t err;
			json_t* file_j = json_loadf(file, 0, &err);
			size_t models_length = json_array_size(file_j);
			if(models_length){
				json_array_append(file_j, params_j);
			}
			else{
				json_decref(file_j);
				file_j = json_array();
				json_array_append(file_j, params_j);
			}
			fclose(file);
			file = fopen(params_filename.c_str(), "w");
			json_dumpf(file_j, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
			fclose(file);
		}
		json_decref(lengths_arr_j);
		json_decref(mixer_arr_j);
		json_decref(params_j);
	}

	void process(const ProcessArgs& args) override
	{
		static simd::float_4 back_fed = simd::float_4::zero();

		float part_A_depth = params[SCALEA_PARAM].getValue();
		part_A_depth = dsp::cubic(part_A_depth);
		float part_B_depth = params[SCALEB_PARAM].getValue();
		part_B_depth = dsp::cubic(part_B_depth);
		parts[0] = params[SWITCH1_PARAM].getValue();
		parts[1] = params[SWITCH2_PARAM].getValue();
		parts[2] = params[SWITCH3_PARAM].getValue();
		parts[3] = params[SWITCH4_PARAM].getValue();
		parts[4] = params[SWITCH5_PARAM].getValue();
		stage1.setScale(parts[0] ? part_B_depth : part_A_depth); 
		stage2.setScale(parts[1] ? part_B_depth : part_A_depth); 
		stage3.setScale(parts[2] ? part_B_depth : part_A_depth); 
		stage4.setScale(parts[3] ? part_B_depth : part_A_depth); 
		stage5.setScale(parts[4] ? part_B_depth : part_A_depth); 

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);
		
		v = v + back_fed;
		v = stage1.process(v);
		v = stage2.process(v);
		v = stage3.process(v);
		v = stage4.process(v);
		
		outputs[LEFT_OUTPUT].setVoltage(v[0]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1]);

		v = stage5.process(v);

		float feedback = params[FEEDBACK_PARAM].getValue();
		back_fed = simd::float_4(feedback)*v;
	}
};

struct GenerateReverbButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		((ReverbDesigner*)this->module)->genReverb();
		VCVButton::onDragStart(e);
	}
};

struct StoreReverbButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		((ReverbDesigner*)this->module)->storeReverb();
		VCVButton::onDragStart(e);
	}
};

struct ReverbDesignerWidget : ModuleWidget {
	ReverbDesignerWidget(ReverbDesigner* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/reverb_designer.svg")));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 11.57)), module, ReverbDesigner::DEL11_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 11.57)), module, ReverbDesigner::DEL21_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 11.57)), module, ReverbDesigner::DEL31_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 11.57)), module, ReverbDesigner::DEL41_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 11.57)), module, ReverbDesigner::DEL51_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 20.876)), module, ReverbDesigner::DEL12_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 20.876)), module, ReverbDesigner::DEL22_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 20.876)), module, ReverbDesigner::DEL32_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 20.876)), module, ReverbDesigner::DEL42_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 20.876)), module, ReverbDesigner::DEL52_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 30.182)), module, ReverbDesigner::DEL13_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 30.182)), module, ReverbDesigner::DEL23_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 30.182)), module, ReverbDesigner::DEL33_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 30.182)), module, ReverbDesigner::DEL43_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 30.182)), module, ReverbDesigner::DEL53_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 39.488)), module, ReverbDesigner::DEL14_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 39.488)), module, ReverbDesigner::DEL24_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 39.488)), module, ReverbDesigner::DEL34_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 39.488)), module, ReverbDesigner::DEL44_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 39.488)), module, ReverbDesigner::DEL54_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.672, 48.795)), module, ReverbDesigner::SCALE1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.116, 48.795)), module, ReverbDesigner::SCALE2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 48.795)), module, ReverbDesigner::SCALE3_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.004, 48.795)), module, ReverbDesigner::SCALE4_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.448, 48.795)), module, ReverbDesigner::SCALE5_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 58.101)), module, ReverbDesigner::MIX11_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 58.101)), module, ReverbDesigner::MIX21_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 58.101)), module, ReverbDesigner::MIX31_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 58.101)), module, ReverbDesigner::MIX41_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 58.101)), module, ReverbDesigner::MIX51_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 67.407)), module, ReverbDesigner::MIX12_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 67.407)), module, ReverbDesigner::MIX22_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 67.407)), module, ReverbDesigner::MIX32_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 67.407)), module, ReverbDesigner::MIX42_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 67.407)), module, ReverbDesigner::MIX52_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 76.713)), module, ReverbDesigner::MIX13_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 76.713)), module, ReverbDesigner::MIX23_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 76.713)), module, ReverbDesigner::MIX33_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 76.713)), module, ReverbDesigner::MIX43_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 76.713)), module, ReverbDesigner::MIX53_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.672, 86.019)), module, ReverbDesigner::MIX14_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(23.116, 86.019)), module, ReverbDesigner::MIX24_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 86.019)), module, ReverbDesigner::MIX34_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.004, 86.019)), module, ReverbDesigner::MIX44_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.448, 86.019)), module, ReverbDesigner::MIX54_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(10.672, 95.326)), module, ReverbDesigner::SWITCH1_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(23.116, 95.326)), module, ReverbDesigner::SWITCH2_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(35.56, 95.326)), module, ReverbDesigner::SWITCH3_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(48.004, 95.326)), module, ReverbDesigner::SWITCH4_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(60.448, 95.326)), module, ReverbDesigner::SWITCH5_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.228, 106.597)), module, ReverbDesigner::SCALEA_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(24.583, 106.597)), module, ReverbDesigner::SCALEB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(56.042, 106.597)), module, ReverbDesigner::FEEDBACK_PARAM));
		addParam(createParamCentered<GenerateReverbButton>(mm2px(Vec(31.667, 118.019)), module, ReverbDesigner::GENERATE_PARAM));
		addParam(createParamCentered<StoreReverbButton>(mm2px(Vec(39.453, 118.019)), module, ReverbDesigner::STORE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, ReverbDesigner::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, ReverbDesigner::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.944, 118.023)), module, ReverbDesigner::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.298, 118.019)), module, ReverbDesigner::RIGHT_OUTPUT));
	}
};


Model* modelReverbDesigner = createModel<ReverbDesigner, ReverbDesignerWidget>("ReverbDesigner");