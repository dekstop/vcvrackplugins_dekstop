#include "rack.hpp"


using namespace rack;


////////////////////
// module widgets
////////////////////

struct TriSEQ3Widget : ModuleWidget {
	TriSEQ3Widget();
	json_t *toJsonData();
	void fromJsonData(json_t *root);
};

struct GateSEQ8Widget : ModuleWidget {
	GateSEQ8Widget();
	json_t *toJsonData();
	void fromJsonData(json_t *root);
};
