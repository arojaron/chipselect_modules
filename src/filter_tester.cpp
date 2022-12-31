#include "plugin.hpp"

#include "components/matched_biquad.hpp"


struct FilterTester : Module {
	enum ParamId {
		LP_PARAM,
		HP_PARAM,
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

	chipselect::LowPass filter;

	FilterTester() 
	: filter(chipselect::LowPass(48000))
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LP_PARAM, 0.08311f, 1.f, 1.f, "Frequency");
		configParam(HP_PARAM, 0.5f, 100, 1.f, "Resonance");
		configInput(LEFT_INPUT, "");
		configInput(RIGHT_INPUT, "");
		configOutput(LEFT_OUTPUT, "");
		configOutput(RIGHT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override
	{
		float filter_param = params[LP_PARAM].getValue();
		filter_param = 21000*filter_param*filter_param*filter_param*filter_param;
		float filter_mod = 5000*0.1*inputs[RIGHT_INPUT].getVoltage();
		float Q_param = params[HP_PARAM].getValue();
		filter.setParams(filter_param + filter_mod, Q_param);

		float in = inputs[LEFT_INPUT].getVoltage();
		outputs[LEFT_OUTPUT].setVoltage(filter.process(in));
	}
};


struct FilterTesterWidget : ModuleWidget {
	FilterTesterWidget(FilterTester* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/FilterTester.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(30.48, 36.539)), module, FilterTester::LP_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(30.48, 73.304)), module, FilterTester::HP_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.336, 118.019)), module, FilterTester::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.691, 118.019)), module, FilterTester::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.269, 118.023)), module, FilterTester::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.624, 118.019)), module, FilterTester::RIGHT_OUTPUT));
	}
};


Model* modelFilterTester = createModel<FilterTester, FilterTesterWidget>("FilterTester");