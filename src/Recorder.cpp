#include "dekstop.hpp"
#include "samplerate.h"
#include "../ext/osdialog/osdialog.h"
#include "write_wav.h"

struct Recorder : Module {
	enum ParamIds {
		RECORD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		AUDIO1_INPUT,
		NUM_INPUTS = AUDIO1_INPUT + 8
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	
	std::string filename;
	WAV_Writer writer;
	bool isRecording = false;
	float recordingLight = 0.0;

	Recorder();
	void step();
	void clear();
	void startRecording();
	void stopRecording();
	void saveAsDialog();
	void openWAV();
	void closeWAV();
};


Recorder::Recorder() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void Recorder::clear() {
	filename = "";
}

void Recorder::startRecording() {
	saveAsDialog();
	if (!filename.empty()) {
		openWAV();
		isRecording = true;
	}
}

void Recorder::stopRecording() {
	if (isRecording) {
		closeWAV();
		isRecording = false;
	}
}

void Recorder::saveAsDialog() {
	std::string dir = filename.empty() ? "." : extractDirectory(filename);
	char *path = osdialog_file(OSDIALOG_SAVE, dir.c_str(), "Output.wav", NULL);
	if (path) {
		filename = path;
		free(path);
	} else {
		filename = "";
	}
}

void Recorder::openWAV() {
	if (!filename.empty()) {
		int result = Audio_WAV_OpenWriter(&writer, filename.c_str(), gSampleRate, 8);
		if (result < 0) {
			char msg[100];
			snprintf(msg, sizeof(msg), "Failed to open WAV file, result = %d\n", result);
			osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
			fprintf(stderr, msg);
		} else {
			isRecording = true;
		}
	}
}

void Recorder::closeWAV() {
	int result = Audio_WAV_CloseWriter(&writer);
	if (result < 0) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Failed to close WAV file, result = %d\n", result);
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
		fprintf(stderr, msg);
	}
	isRecording = false;
}

void Recorder::step() {
	recordingLight = isRecording ? 1.0 : 0.0;

	if (isRecording) {
		// Read input samples
		float indata[8];
		short outdata[8];
		for (int i = 0; i < 8; i++) {
			indata[i] = getf(inputs[AUDIO1_INPUT + i]) / 5.0;
		}
		src_float_to_short_array(indata, outdata, 8);
		// Write to file
		// TODO: use a mutex; this writer might get closed at any time
		int result = Audio_WAV_WriteShorts(&writer, outdata, 8);
		if (result < 0) {
			stopRecording();

			char msg[100];
			snprintf(msg, sizeof(msg), "Failed to write WAV file, result = %d\n", result);
			// osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
			fprintf(stderr, msg);
		}
	}
}

struct RecordButton : LEDButton {
	Recorder *recorder;
	SchmittTrigger recordTrigger;
	
	void onChange() {
		printf("onChange\n");
		if (recordTrigger.process(value)) {
			onAction();
		}
	}
	void onAction() {
		printf("onAction\n");
		if (!recorder->isRecording) {
			recorder->startRecording();
		} else {
			recorder->stopRecording();
		}
	}
};


RecorderWidget::RecorderWidget() {
	Recorder *module = new Recorder();
	setModule(module);
	box.size = Vec(15*6+5, 380);

	{
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		addChild(panel);
	}

	float margin = 5;
	float labelHeight = 15;
	float yPos = margin;
	float xPos = margin;

	{
		Label *label = new Label();
		label->box.pos = Vec(xPos, yPos);
		label->text = "Record";
		addChild(label);
		yPos += labelHeight + margin;

		xPos = 35;
		yPos += 2*margin;
		ParamWidget *recordButton = createParam<RecordButton>(Vec(xPos, yPos-1), module, Recorder::RECORD_PARAM, 0.0, 1.0, 0.0);
		RecordButton *btn = dynamic_cast<RecordButton*>(recordButton);
		btn->recorder = dynamic_cast<Recorder*>(module);
		addParam(recordButton);
		addChild(createValueLight<SmallLight<RedValueLight>>(Vec(xPos+5, yPos+4), &module->recordingLight));
		xPos = margin;
		yPos += recordButton->box.size.y + 3*margin;
	}

	{
		Label *label = new Label();
		label->box.pos = Vec(margin, yPos);
		label->text = "Channels";
		addChild(label);
		yPos += labelHeight + margin;
	}

	yPos += 5;
	xPos = 10;
	for (int i = 0; i < 8; i++) {
		addInput(createInput<PJ3410Port>(Vec(xPos, yPos), module, Recorder::AUDIO1_INPUT + i));
		Label *label = new Label();
		label->box.pos = Vec(xPos + 4, yPos + 28);
		label->text = stringf("%d", i + 1);
		addChild(label);

		if (i % 2 ==0) {
			xPos += 37 + margin;
		} else {
			xPos = 10;
			yPos += 40 + margin;
		}
	}
}
