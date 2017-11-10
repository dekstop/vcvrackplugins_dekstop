#include "dekstop.hpp"
#include <math.h>

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "dekstop";
	plugin->name = "dekstop";
	plugin->homepageUrl = "https://github.com/dekstop/vcvrackplugins_dekstop";
	createModel<TriSEQ3Widget>(plugin, "TriSEQ3", "Tri-state SEQ-3");
	createModel<GateSEQ8Widget>(plugin, "GateSEQ8", "Gate SEQ-8");
	createModel<Recorder2Widget>(plugin, "Recorder2", "Recorder 2");
	createModel<Recorder8Widget>(plugin, "Recorder8", "Recorder 8");
}
