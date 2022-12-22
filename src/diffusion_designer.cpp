#include "plugin.hpp"

#include "rev_diffusion_stage.hpp"

static simd::float_4 const init_lengths = simd::float_4::zero();
static simd::float_4 const init_normal = simd::float_4(1, 1, 1, 1);

struct DiffusionDesigner : Module {
	enum ParamId {
		DEL11_PARAM,
		DEL21_PARAM,
		DEL31_PARAM,
		DEL41_PARAM,
		DEL12_PARAM,
		DEL22_PARAM,
		DEL32_PARAM,
		DEL42_PARAM,
		DEL13_PARAM,
		DEL23_PARAM,
		DEL33_PARAM,
		DEL43_PARAM,
		DEL14_PARAM,
		DEL24_PARAM,
		DEL34_PARAM,
		DEL44_PARAM,
		MIX11_PARAM,
		MIX21_PARAM,
		MIX31_PARAM,
		MIX41_PARAM,
		MIX12_PARAM,
		MIX22_PARAM,
		MIX32_PARAM,
		MIX42_PARAM,
		MIX13_PARAM,
		MIX23_PARAM,
		MIX33_PARAM,
		MIX43_PARAM,
		MIX14_PARAM,
		MIX24_PARAM,
		MIX34_PARAM,
		MIX44_PARAM,
		DIFF_PARAM,
		FEEDBACK_PARAM,
		GENERATE_PARAM,
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
	
	DiffusionStage stage1;
	DiffusionStage stage2;
	DiffusionStage stage3;
	DiffusionStage stage4;
	bool gen = true;

	DiffusionDesigner() 
	: stage1(DiffusionStage(init_lengths, init_normal, FS)),
	  stage2(DiffusionStage(init_lengths, init_normal, FS)),
	  stage3(DiffusionStage(init_lengths, init_normal, FS)),
	  stage4(DiffusionStage(init_lengths, init_normal, FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DEL11_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL21_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL31_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL41_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL12_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL22_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL32_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL42_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL13_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL23_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL33_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL43_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL14_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL24_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL34_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEL44_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MIX11_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX21_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX31_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX41_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX12_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX22_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX32_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX42_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX13_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX23_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX33_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX43_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX14_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX24_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX34_PARAM, -1.f, 1.f, 1.f, "");
		configParam(MIX44_PARAM, -1.f, 1.f, 1.f, "");
		configParam(DIFF_PARAM, 0.f, 1.f, 0.5, "");
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5, "");
		configParam(GENERATE_PARAM, 0.f, 1.f, 0.f, "");
		configInput(LEFT_INPUT, "");
		configInput(RIGHT_INPUT, "");
		configOutput(LEFT_OUTPUT, "");
		configOutput(RIGHT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override
	{
		if(gen){
			simd::float_4 lengths1 = simd::float_4(params[DEL11_PARAM].getValue(), params[DEL12_PARAM].getValue(), params[DEL13_PARAM].getValue(), params[DEL14_PARAM].getValue());
			simd::float_4 lengths2 = simd::float_4(params[DEL21_PARAM].getValue(), params[DEL22_PARAM].getValue(), params[DEL23_PARAM].getValue(), params[DEL24_PARAM].getValue());
			simd::float_4 lengths3 = simd::float_4(params[DEL31_PARAM].getValue(), params[DEL32_PARAM].getValue(), params[DEL33_PARAM].getValue(), params[DEL34_PARAM].getValue());
			simd::float_4 lengths4 = simd::float_4(params[DEL41_PARAM].getValue(), params[DEL42_PARAM].getValue(), params[DEL43_PARAM].getValue(), params[DEL44_PARAM].getValue());
			simd::float_4 normals1 = simd::float_4(params[MIX11_PARAM].getValue(), params[MIX12_PARAM].getValue(), params[MIX13_PARAM].getValue(), params[MIX14_PARAM].getValue());
			simd::float_4 normals2 = simd::float_4(params[MIX21_PARAM].getValue(), params[MIX22_PARAM].getValue(), params[MIX23_PARAM].getValue(), params[MIX24_PARAM].getValue());
			simd::float_4 normals3 = simd::float_4(params[MIX31_PARAM].getValue(), params[MIX32_PARAM].getValue(), params[MIX33_PARAM].getValue(), params[MIX34_PARAM].getValue());
			simd::float_4 normals4 = simd::float_4(params[MIX41_PARAM].getValue(), params[MIX42_PARAM].getValue(), params[MIX43_PARAM].getValue(), params[MIX44_PARAM].getValue());
			stage1 = DiffusionStage(lengths1, normals1, FS);
			stage2 = DiffusionStage(lengths2, normals2, FS);
			stage3 = DiffusionStage(lengths3, normals3, FS);
			stage4 = DiffusionStage(lengths4, normals4, FS);
			gen = false;
		}

		static simd::float_4 back_fed = simd::float_4::zero();

		float diff_depth = params[DIFF_PARAM].getValue();
		diff_depth = dsp::cubic(diff_depth);
		stage1.setScale(diff_depth);
        stage2.setScale(diff_depth);
        stage3.setScale(diff_depth);
        stage4.setScale(diff_depth);

		float left = inputs[LEFT_INPUT].getVoltage();
		float right = inputs[RIGHT_INPUT].isConnected() ? inputs[RIGHT_INPUT].getVoltage() : left;
		simd::float_4 v = simd::float_4(left, right, left, right);
		
		v = v + back_fed;
		v = stage1.process(v);
		v = stage2.process(v);
		v = stage3.process(v);
		v = stage4.process(v);

		outputs[LEFT_OUTPUT].setVoltage(v[0] + v[2]);
		outputs[RIGHT_OUTPUT].setVoltage(v[1] + v[3]);

		float feedback = params[FEEDBACK_PARAM].getValue();
		back_fed = simd::float_4(feedback)*v;
	}
};

struct GenerateButton : VCVButton{
	void onDragStart(const DragStartEvent& e) override
	{
		((DiffusionDesigner*)this->module)->gen = true;
	}
};


struct DiffusionDesignerWidget : ModuleWidget {
	DiffusionDesignerWidget(DiffusionDesigner* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/diffusion_designer.svg")));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 16.633)), module, DiffusionDesigner::DEL11_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 16.633)), module, DiffusionDesigner::DEL21_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 16.633)), module, DiffusionDesigner::DEL31_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 16.633)), module, DiffusionDesigner::DEL41_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 25.183)), module, DiffusionDesigner::DEL12_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 25.183)), module, DiffusionDesigner::DEL22_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 25.183)), module, DiffusionDesigner::DEL32_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 25.183)), module, DiffusionDesigner::DEL42_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 33.732)), module, DiffusionDesigner::DEL13_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 33.732)), module, DiffusionDesigner::DEL23_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 33.732)), module, DiffusionDesigner::DEL33_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 33.732)), module, DiffusionDesigner::DEL43_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 42.282)), module, DiffusionDesigner::DEL14_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 42.282)), module, DiffusionDesigner::DEL24_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 42.282)), module, DiffusionDesigner::DEL34_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 42.282)), module, DiffusionDesigner::DEL44_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 59.381)), module, DiffusionDesigner::MIX11_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 59.381)), module, DiffusionDesigner::MIX21_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 59.381)), module, DiffusionDesigner::MIX31_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 59.381)), module, DiffusionDesigner::MIX41_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 67.931)), module, DiffusionDesigner::MIX12_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 67.931)), module, DiffusionDesigner::MIX22_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 67.931)), module, DiffusionDesigner::MIX32_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 67.931)), module, DiffusionDesigner::MIX42_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 76.481)), module, DiffusionDesigner::MIX13_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 76.481)), module, DiffusionDesigner::MIX23_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 76.481)), module, DiffusionDesigner::MIX33_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 76.481)), module, DiffusionDesigner::MIX43_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.814, 85.031)), module, DiffusionDesigner::MIX14_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.258, 85.031)), module, DiffusionDesigner::MIX24_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(36.702, 85.031)), module, DiffusionDesigner::MIX34_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.146, 85.031)), module, DiffusionDesigner::MIX44_PARAM));
		addParam(createParamCentered<GenerateButton>(mm2px(Vec(30.48, 105.04)), module, DiffusionDesigner::GENERATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.928, 98.425)), module, DiffusionDesigner::DIFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.032, 98.425)), module, DiffusionDesigner::FEEDBACK_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, DiffusionDesigner::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, DiffusionDesigner::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, DiffusionDesigner::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, DiffusionDesigner::RIGHT_OUTPUT));
	}
};


Model* modelDiffusionDesigner = createModel<DiffusionDesigner, DiffusionDesignerWidget>("diffusion_designer");