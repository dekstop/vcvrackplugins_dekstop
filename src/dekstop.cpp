#include "dekstop.hpp"
#include <math.h>
#include "dsp.hpp"


struct DekstopPlugin : Plugin {
	DekstopPlugin() {
		slug = "dekstop";
		name = "dekstop";
		createModel<TriSEQ3Widget>(this, "TriSEQ3", "Tri-state SEQ-3");
		createModel<GateSEQ8Widget>(this, "GateSEQ8", "Gate SEQ-8");
	}
};


Plugin *init() {
	return new DekstopPlugin();
}
