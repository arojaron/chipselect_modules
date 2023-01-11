#include "plugin.hpp"

#include "components\transient_detection.hpp"


struct Demoer : Module {
	enum ParamId {
		A_PARAM,
		B_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float FS = 48000.f;
	cs::TransientDetector detector;

	Demoer()
	: detector(cs::TransientDetector(FS))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(A_PARAM, 0.f, 1.f, 0.f, "A");
		configParam(B_PARAM, 0.f, 1.f, 0.f, "B");
		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configInput(C_INPUT, "C");
		configInput(D_INPUT, "D");
		configOutput(A_OUTPUT, "A");
		configOutput(B_OUTPUT, "B");
		configOutput(C_OUTPUT, "C");
		configOutput(D_OUTPUT, "D");
	}

	void process(const ProcessArgs& args) override
	{
		float signal = inputs[A_INPUT].getVoltage();
		outputs[C_OUTPUT].setVoltage(detector.process(signal));
		outputs[A_OUTPUT].setVoltage(detector.following);
		outputs[B_OUTPUT].setVoltage(detector.lagging);
	}
};


struct DemoerWidget : ModuleWidget {
	DemoerWidget(Demoer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/demoer.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 15.856)), module, Demoer::A_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 45.248)), module, Demoer::B_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 84.235)), module, Demoer::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 95.496)), module, Demoer::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 106.757)), module, Demoer::C_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.607, 118.019)), module, Demoer::D_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 84.235)), module, Demoer::A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 95.496)), module, Demoer::B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 106.757)), module, Demoer::C_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.873, 118.019)), module, Demoer::D_OUTPUT));
	}
};


Model* modelDemoer = createModel<Demoer, DemoerWidget>("Demoer");