#include <thread>

#include "dekstop.hpp"
#include "samplerate.h"
#include "../ext/osdialog/osdialog.h"
#include "write_wav.h"
#include "../../../include/dsp/ringbuffer.hpp"
#include "../../../include/dsp/frame.hpp"
#include "../../../include/dsp/digital.hpp"

#define BLOCKSIZE 1024
#define BUFFERSIZE 32*BLOCKSIZE

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

	std::mutex mutex;
	std::thread thread;
	RingBuffer<Frame<8>, BUFFERSIZE> buffer;
	short writeBuffer[8*BUFFERSIZE];

	Recorder() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	~Recorder();
	void step();
	void clear();
	void startRecording();
	void stopRecording();
	void saveAsDialog();
	void openWAV();
	void closeWAV();
	void recorderRun();
};


Recorder::~Recorder() {
	if (isRecording) stopRecording();
}

void Recorder::clear() {
	filename = "";
}

void Recorder::startRecording() {
	saveAsDialog();
	if (!filename.empty()) {
		openWAV();
		isRecording = true;
		thread = std::thread(&Recorder::recorderRun, this);
	}
}

void Recorder::stopRecording() {
	isRecording = false;
	thread.join();
	closeWAV();
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
		fprintf(stdout, "Recording to %s\n", filename.c_str());
		int result = Audio_WAV_OpenWriter(&writer, filename.c_str(), gSampleRate, 8);
		if (result < 0) {
			isRecording = false;
			char msg[100];
			snprintf(msg, sizeof(msg), "Failed to open WAV file, result = %d\n", result);
			osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
			fprintf(stderr, msg);
		} 
	}
}

void Recorder::closeWAV() {
	fprintf(stdout, "Stopping the recording.\n");
	int result = Audio_WAV_CloseWriter(&writer);
	if (result < 0) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Failed to close WAV file, result = %d\n", result);
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
		fprintf(stderr, msg);
	}
	isRecording = false;
}

// Run in a separate thread
void Recorder::recorderRun() {
	while (isRecording) {
		// Wake up a few times a second, often enough to never overflow the buffer.
		float sleepTime = (1.0 * BUFFERSIZE / gSampleRate) / 2.0;
		std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
		if (buffer.full()) {
			fprintf(stderr, "Recording buffer overflow. Can't write quickly enough to disk. Current buffer size: %d\n", BUFFERSIZE);
		}
		// Check if there is data
		int numFrames = buffer.size();
		if (numFrames > 0) {
			// Convert float frames to shorts
			{
				std::lock_guard<std::mutex> lock(mutex); // Lock during conversion
				src_float_to_short_array(static_cast<float*>(buffer.data[0].samples), writeBuffer, 8*numFrames);
				buffer.start = 0;
				buffer.end = 0;
			}

			fprintf(stdout, "Writing %d frames to disk\n", numFrames);
			int result = Audio_WAV_WriteShorts(&writer, writeBuffer, 8*numFrames);
			if (result < 0) {
				stopRecording();

				char msg[100];
				snprintf(msg, sizeof(msg), "Failed to write WAV file, result = %d\n", result);
				osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg);
				fprintf(stderr, msg);
			}
		}
	}
}

void Recorder::step() {
	recordingLight = isRecording ? 1.0 : 0.0;
	if (isRecording) {
		// Read input samples into recording buffer
		std::lock_guard<std::mutex> lock(mutex);
		if (!buffer.full()) {
			Frame<8> f;
			for (int i = 0; i < 8; i++) {
				f.samples[i] = inputs[AUDIO1_INPUT + i].value / 5.0;
			}
			buffer.push(f);
		}
	}
}

struct RecordButton : LEDButton {
	Recorder *recorder;
	SchmittTrigger recordTrigger;
	
	void onChange() {
		if (recordTrigger.process(value)) {
			onPress();
		}
	}
	void onPress() {
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
		label->text = "Recorder";
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
