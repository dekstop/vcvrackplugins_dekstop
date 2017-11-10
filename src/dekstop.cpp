#include "dekstop.hpp"
#include <math.h>

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "dekstop";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->addModel(createModel<TriSEQ3Widget>("dekstop", "dekstop", "TriSEQ3", "Tri-state SEQ-3"));
	p->addModel(createModel<GateSEQ8Widget>("dekstop", "dekstop", "GateSEQ8", "Gate SEQ-8"));
	p->addModel(createModel<Recorder2Widget>("dekstop", "dekstop", "Recorder2", "Recorder 2"));
	p->addModel(createModel<Recorder8Widget>("dekstop", "dekstop", "Recorder8", "Recorder 8"));
}
