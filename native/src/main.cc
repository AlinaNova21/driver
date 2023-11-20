// Author: Marcel Laverdet <https://github.com/laverdet>
#include "napi.h"
#include <uv.h>
#include <array>
#include <memory>
#include <limits>
#include "pf.h"

using namespace Napi;

namespace screeps {

	// Init 2 Pathfinders per thread. We do 2 here because sometimes recursive calls to the path
	// finder are useful. Any more than 2 deep recursion will have to allocate a new path finder at a
	// cost of 2.16mb(!)
	thread_local std::array<path_finder_t, 2> path_finders;
	uint8_t room_info_t::cost_matrix0[2500] = { 0 };

	Value search(const CallbackInfo& info) {
		// Find an inactive path finder
		path_finder_t* pf = nullptr;
		std::unique_ptr<path_finder_t> pf_holder;
		for (auto& ii : path_finders) {
			if (!ii.is_in_use()) {
				pf = &ii;
				break;
			}
		}
		if (pf == nullptr) {
			pf_holder = std::make_unique<path_finder_t>();
			pf = pf_holder.get();
		}

		// Get the values from v8 and run the search
		cost_t plain_cost = info[3].As<Number>().Uint32Value();
		cost_t swamp_cost = info[4].As<Number>().Uint32Value();
		uint8_t max_rooms = info[5].As<Number>().Uint32Value();
		uint32_t max_ops = info[6].As<Number>().Uint32Value();
		uint32_t max_cost = info[7].As<Number>().Uint32Value();
		bool flee = info[8].As<Boolean>().Value();
		double heuristic_weight = info[9].As<Number>().DoubleValue();
		return pf->search(
			info.Env(),
			info[0], info[1].As<Array>(), // origin + goals
			info[2].As<Function>(), // callback
			plain_cost, swamp_cost,
			max_rooms, max_ops, max_cost,
			flee,
			heuristic_weight
		);
	}

	void load_terrain(const CallbackInfo& info) {
		
		path_finder_t::load_terrain(info.Env(), info[0].As<Array>());
	}
};

Object init(Env env, Object exports) {
	exports.Set("search", Function::New<screeps::search>(env));
	exports.Set("loadTerrain", Function::New<screeps::load_terrain>(env));
	exports.Set("version", Number::New(env, 11));
	return exports;
}
NODE_API_MODULE(native, init);
