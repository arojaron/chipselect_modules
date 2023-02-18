#include "plugin.hpp"
#include "components/tuned_envelope.hpp"


struct Pulse : Module {
	enum ParamId {
		LENGTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VPOCT_INPUT,
		VCA_INPUT,
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VCA_OUTPUT,
		ENVELOPE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};


	cs::TunedDecayEnvelope<float> processor;
	dsp::BooleanTrigger trigger;

	Pulse() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LENGTH_PARAM, std::log2(2.f), std::log2(1000.f), std::log2(100.f), "Frequency", "Hz", 2);
		configInput(VPOCT_INPUT, "Tune V/Oct");
		configInput(VCA_INPUT, "VCA");
		configInput(TRIGGER_INPUT, "Trigger");
		configOutput(VCA_OUTPUT, "VCA");
		configOutput(ENVELOPE_OUTPUT, "Envelope");
	}

	void process(const ProcessArgs& args) override
	{
		float freq = dsp::approxExp2_taylor5(params[LENGTH_PARAM].getValue());
		float freq_mod = dsp::approxExp2_taylor5(inputs[VPOCT_INPUT].getVoltage());

		processor.setFrequency(freq * freq_mod);

		float triggered = trigger.process(0.f < inputs[TRIGGER_INPUT].getVoltage());
		float env = processor.process(args.sampleTime, triggered ? 1.f : 0.f);

		float vca_out = env * inputs[VCA_INPUT].getVoltage();

		outputs[ENVELOPE_OUTPUT].setVoltage(10.f * env);
		outputs[VCA_OUTPUT].setVoltage(vca_out);
	}
};


struct PulseWidget : ModuleWidget {
	PulseWidget(Pulse* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/pulse.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(15.24, 37.771)), module, Pulse::LENGTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.184, 73.619)), module, Pulse::VPOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.484, 100.221)), module, Pulse::VCA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.224, 118.283)), module, Pulse::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.271, 102.942)), module, Pulse::VCA_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.257, 116.693)), module, Pulse::ENVELOPE_OUTPUT));
	}
};


Model* modelPulse = createModel<Pulse, PulseWidget>("Pulse");