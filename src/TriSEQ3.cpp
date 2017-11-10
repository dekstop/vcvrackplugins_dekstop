#include "dekstop.hpp"
#include "dsp/digital.hpp"

struct TriSEQ3 : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		ROW1_PARAM,
		ROW2_PARAM = ROW1_PARAM + 8,
		ROW3_PARAM = ROW2_PARAM + 8,
		GATE_PARAM = ROW3_PARAM + 8,
		NUM_PARAMS = GATE_PARAM + 8
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		ROW1_OUTPUT,
		ROW2_OUTPUT,
		ROW3_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS = GATE_OUTPUT + 8
	};

	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	float phase = 0.0;
	int index = 0;
	SchmittTrigger gateTriggers[8];
	bool gateState[8] = {};
	float stepLights[8] = {};

	// Lights
	float runningLight = 0.0;
	float resetLight = 0.0;
	float gatesLight = 0.0;
	float rowLights[3] = {};
	float gateLights[8] = {};

	TriSEQ3() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();

	json_t *toJson() {
		json_t *rootJ = json_object();

		json_t *gatesJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t *gateJ = json_integer((int) gateState[i]);
			json_array_append_new(gatesJ, gateJ);
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		return rootJ;
	}

	void fromJson(json_t *rootJ) {
		json_t *gatesJ = json_object_get(rootJ, "gates");
		for (int i = 0; i < 8; i++) {
			json_t *gateJ = json_array_get(gatesJ, i);
			gateState[i] = !!json_integer_value(gateJ);
		}
	}

#ifdef v_050_dev
	void reset() {
#else
	void initialize() {
#endif
		for (int i = 0; i < 8; i++) {
			gateState[i] = false;
		}
	}

	void randomize() {
		for (int i = 0; i < 8; i++) {
			gateState[i] = (randomf() > 0.5);
		}
	}
};


void TriSEQ3::step() {
	#ifdef v_050_dev
	float gSampleRate = engineGetSampleRate();
	#endif
	const float lightLambda = 0.1;
	// Run
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
	}
	runningLight = running ? 1.0 : 0.0;

	bool nextStep = false;

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].active) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
				phase = 0.0;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime / gSampleRate;
			if (phase >= 1.0) {
				phase -= 1.0;
				nextStep = true;
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
		phase = 0.0;
		index = 999;
		nextStep = true;
		resetLight = 1.0;
	}

	if (nextStep) {
		// Advance step
		int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, 8);
		index += 1;
		if (index >= numSteps) {
			index = 0;
		}
		stepLights[index] = 1.0;
	}

	resetLight -= resetLight / lightLambda / gSampleRate;

	// Gate buttons
	for (int i = 0; i < 8; i++) {
		if (gateTriggers[i].process(params[GATE_PARAM + i].value)) {
			gateState[i] = !gateState[i];
		}
		float gate = (i == index && gateState[i] >= 1.0) ? 10.0 : 0.0;
		outputs[GATE_OUTPUT + i].value = gate;
		stepLights[i] -= stepLights[i] / lightLambda / gSampleRate;
		gateLights[i] = (gateState[i] >= 1.0) ? 1.0 - stepLights[i] : stepLights[i];
	}

	// Rows
	float row1 = params[ROW1_PARAM + index].value;
	float row2 = params[ROW2_PARAM + index].value;
	float row3 = params[ROW3_PARAM + index].value;
	float gates = (gateState[index] >= 1.0) && !nextStep ? 10.0 : 0.0;
	outputs[ROW1_OUTPUT].value = row1;
	outputs[ROW2_OUTPUT].value = row2;
	outputs[ROW3_OUTPUT].value = row3;
	outputs[GATES_OUTPUT].value = gates;
	gatesLight = (gateState[index] >= 1.0) ? 1.0 : 0.0;
	rowLights[0] = row1;
	rowLights[1] = row2;
	rowLights[2] = row3;
}


TriSEQ3Widget::TriSEQ3Widget() {
	TriSEQ3 *module = new TriSEQ3();
	setModule(module);
	box.size = Vec(15*22, 380);

	{
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/dekstop/res/TriSEQ3.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(17, 56), module, TriSEQ3::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<LEDButton>(Vec(60, 61-1), module, TriSEQ3::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(60+5, 61+4), &module->runningLight));
	addParam(createParam<LEDButton>(Vec(98, 61-1), module, TriSEQ3::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(98+5, 61+4), &module->resetLight));
	addParam(createParam<Davies1900hSmallBlackSnapKnob>(Vec(132, 56), module, TriSEQ3::STEPS_PARAM, 1.0, 8.0, 8.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(180.5, 65), &module->gatesLight));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(218.5, 65), &module->rowLights[0]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(257, 65), &module->rowLights[1]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(295.5, 65), &module->rowLights[2]));

	static const float portX[8] = {19, 57, 96, 134, 173, 211, 250, 288};
	addInput(createInput<PJ301MPort>(Vec(portX[0]-1, 99-1), module, TriSEQ3::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[1]-1, 99-1), module, TriSEQ3::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[2]-1, 99-1), module, TriSEQ3::RESET_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[3]-1, 99-1), module, TriSEQ3::STEPS_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(portX[4]-1, 99-1), module, TriSEQ3::GATES_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(portX[5]-1, 99-1), module, TriSEQ3::ROW1_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(portX[6]-1, 99-1), module, TriSEQ3::ROW2_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(portX[7]-1, 99-1), module, TriSEQ3::ROW3_OUTPUT));

	for (int i = 0; i < 8; i++) {
		addParam(createParam<NKK>(Vec(portX[i]-2, 152), module, TriSEQ3::ROW1_PARAM + i, 0.0, 2.0, 0.0));
		addParam(createParam<NKK>(Vec(portX[i]-2, 190), module, TriSEQ3::ROW2_PARAM + i, 0.0, 2.0, 0.0));
		addParam(createParam<NKK>(Vec(portX[i]-2, 229), module, TriSEQ3::ROW3_PARAM + i, 0.0, 2.0, 0.0));
		addParam(createParam<LEDButton>(Vec(portX[i]+2, 278-1), module, TriSEQ3::GATE_PARAM + i, 0.0, 1.0, 0.0));
		addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(portX[i]+7, 278+4), &module->gateLights[i]));
		addOutput(createOutput<PJ301MPort>(Vec(portX[i]-1, 308-1), module, TriSEQ3::GATE_OUTPUT + i));
	}
}
