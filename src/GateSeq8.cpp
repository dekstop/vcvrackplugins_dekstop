#include "dekstop.hpp"

const int NUM_STEPS = 12;
const int NUM_CHANNELS = 8;
const int NUM_GATES = NUM_STEPS * NUM_CHANNELS;

struct GateSEQ8 : Module {
	
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		GATE1_PARAM,
		NUM_PARAMS = GATE1_PARAM + NUM_GATES
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE1_OUTPUT,
		NUM_OUTPUTS = GATE1_OUTPUT + NUM_CHANNELS
	};

	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	float multiplier = 1.0;
	float phase = 0.0;
	int index = 0;
	SchmittTrigger gateTriggers[NUM_GATES];
	bool gateState[NUM_GATES] = {};
	float stepLights[NUM_GATES] = {};

	// Lights
	float runningLight = 0.0;
	float resetLight = 0.0;
	float gateLights[NUM_GATES] = {};

	GateSEQ8();
	void step();

	json_t *toJson() {
		json_t *rootJ = json_object();

		// Clock multiplier
		json_t *multiplierJ = json_real(multiplier);
		json_object_set_new(rootJ, "multiplier", multiplierJ);

		// Gate values
		json_t *gatesJ = json_array();
		for (int i = 0; i < NUM_GATES; i++) {
			json_t *gateJ = json_integer((int) gateState[i]);
			json_array_append_new(gatesJ, gateJ);
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		return rootJ;
	}

	void fromJson(json_t *rootJ) {
		// Clock multiplier
		json_t *multiplierJ = json_object_get(rootJ, "multiplier");
		if (!multiplierJ) {
			multiplier = 1.0;
		} else {
			multiplier = (float)json_real_value(multiplierJ);
		}

		// Gate values
		json_t *gatesJ = json_object_get(rootJ, "gates");
		for (int i = 0; i < NUM_GATES; i++) {
			json_t *gateJ = json_array_get(gatesJ, i);
			gateState[i] = !!json_integer_value(gateJ);
		}
	}

	void initialize() {
		for (int i = 0; i < NUM_GATES; i++) {
			gateState[i] = false;
		}
	}

	void randomize() {
		for (int i = 0; i < NUM_GATES; i++) {
			gateState[i] = (randomf() > 0.5);
		}
	}
};


GateSEQ8::GateSEQ8() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void GateSEQ8::step() {
	const float lightLambda = 0.1;
	// Run
	if (runningTrigger.process(params[RUN_PARAM])) {
		running = !running;
	}
	runningLight = running ? 1.0 : 0.0;

	bool nextStep = false;

	if (running) {
		if (inputs[EXT_CLOCK_INPUT]) {
			// External clock
			if (clockTrigger.process(*inputs[EXT_CLOCK_INPUT])) {
				phase = 0.0;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0, params[CLOCK_PARAM] + getf(inputs[CLOCK_INPUT]));
			clockTime = clockTime * multiplier;
			phase += clockTime / gSampleRate;
			if (phase >= 1.0) {
				phase -= 1.0;
				nextStep = true;
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM] + getf(inputs[RESET_INPUT]))) {
		phase = 0.0;
		index = 999;
		nextStep = true;
		resetLight = 1.0;
	}

	if (nextStep) {
		// Advance step
		int numSteps = clampi(roundf(params[STEPS_PARAM] + getf(inputs[STEPS_INPUT])), 1, NUM_STEPS);
		index += 1;
		if (index >= numSteps) {
			index = 0;
		}
		for (int y = 0; y < NUM_CHANNELS; y++) {
			stepLights[y*NUM_STEPS + index] = 1.0;
		}
	}

	resetLight -= resetLight / lightLambda / gSampleRate;

	// Gate buttons
	for (int i = 0; i < NUM_GATES; i++) {
		if (gateTriggers[i].process(params[GATE1_PARAM + i])) {
			gateState[i] = !gateState[i];
		}
		stepLights[i] -= stepLights[i] / lightLambda / gSampleRate;
		gateLights[i] = (gateState[i] >= 1.0) ? 1.0 - stepLights[i] : stepLights[i];
	}
	for (int y = 0; y < NUM_CHANNELS; y++) {
		float gate = (gateState[y*NUM_STEPS + index] >= 1.0) ? 10.0 : 0.0;
		setf(outputs[GATE1_OUTPUT + y], gate);
	}
}


struct ClockMultiplierItem : MenuItem {
	GateSEQ8 *gateSEQ8;
	float multiplier;
	void onAction() {
		gateSEQ8->multiplier = multiplier;
	}
};

struct ClockMultiplierChoice : ChoiceButton {
	GateSEQ8 *gateSEQ8;
	void onAction() {
		Menu *menu = gScene->createMenu();
		menu->box.pos = getAbsolutePos().plus(Vec(0, box.size.y));
		menu->box.size.x = box.size.x;

		const float multipliers[8] = {0.25, 0.3, 0.5, 0.75, 
																	1.0, 1.5, 2.0, 3.0};
		const std::string labels[8] = {"1/4", "1/3", "1/2", "3/4", 
																	 "1/1", "3/2", "2/1", "3/1"};
		int multipliersLen = sizeof(multipliers) / sizeof(multipliers[0]);
		for (int i = 0; i < multipliersLen; i++) {
			ClockMultiplierItem *item = new ClockMultiplierItem();
			item->gateSEQ8 = gateSEQ8;
			item->multiplier = multipliers[i];
			item->text = stringf("%.2f (%s)", multipliers[i], labels[i].c_str());
			menu->pushChild(item);
		}
	}
	void step() {
		this->text = stringf("%.2f", gateSEQ8->multiplier);
	}
};

GateSEQ8Widget::GateSEQ8Widget() {
	GateSEQ8 *module = new GateSEQ8();
	setModule(module);
	box.size = Vec(360, 380);

	{
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/dekstop/res/GateSEQ8.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(17, 56), module, GateSEQ8::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<LEDButton>(Vec(60, 61-1), module, GateSEQ8::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(60+5, 61+4), &module->runningLight));
	addParam(createParam<LEDButton>(Vec(98, 61-1), module, GateSEQ8::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(98+5, 61+4), &module->resetLight));
	addParam(createParam<Davies1900hSmallBlackSnapKnob>(Vec(132, 56), module, GateSEQ8::STEPS_PARAM, 1.0, NUM_STEPS, NUM_STEPS));

	static const float portX[8] = {19, 57, 96, 134, 173, 211, 250, 288};
	addInput(createInput<PJ301MPort>(Vec(portX[0]-1, 99-1), module, GateSEQ8::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[1]-1, 99-1), module, GateSEQ8::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[2]-1, 99-1), module, GateSEQ8::RESET_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[3]-1, 99-1), module, GateSEQ8::STEPS_INPUT));

	{
		Label *label = new Label();
		label->box.pos = Vec(200, 55);
		label->text = "Clock multiplier";
		addChild(label);

		ClockMultiplierChoice *choice = new ClockMultiplierChoice();
		choice->gateSEQ8 = dynamic_cast<GateSEQ8*>(module);
		choice->box.pos = Vec(200, 70);
		choice->box.size.x = 100;
		addChild(choice);
	}

	for (int y = 0; y < NUM_CHANNELS; y++) {
		for (int x = 0; x < NUM_STEPS; x++) {
			int i = y*NUM_STEPS+x;
			addParam(createParam<LEDButton>(Vec(22 + x*25, 155+y*25+3), module, GateSEQ8::GATE1_PARAM + i, 0.0, 1.0, 0.0));
			addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(27 + x*25, 155+y*25+8), &module->gateLights[i]));
		}
		addOutput(createOutput<PJ301MPort>(Vec(320, 155+y*25), module, GateSEQ8::GATE1_OUTPUT + y));
	}
}
