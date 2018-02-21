#include "dekstop.hpp"
#include <math.h>

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = TOSTRING(SLUG);;
	p->version = TOSTRING(VERSION);
	p->website = "https://github.com/dekstop/vcvrackplugins_dekstop";

	// p->addModel(createModel<GateSEQ8Widget>("dekstop", "GateSEQ8", "Gate SEQ-8", SEQUENCER_TAG));
	p->addModel(modelGateSEQ8);
	p->addModel(modelTriSEQ3);
	p->addModel(modelRecorder2);
	p->addModel(modelRecorder8);
}
