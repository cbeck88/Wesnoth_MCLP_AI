/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */

#include "ai.hpp"

#include "../actions.hpp"
#include "../manager.hpp"
#include "../formula/ai.hpp"

#include "../../array.hpp"
#include "../../dialogs.hpp"
#include "../../game_events/pump.hpp"
#include "../../gamestatus.hpp"
#include "../../log.hpp"
#include "../../mouse_handler_base.hpp"
#include "../../resources.hpp"
#include "../../terrain_filter.hpp"
#include "../../unit_display.hpp"
#include "../../wml_exception.hpp"

#include "../../pathfind/pathfind.hpp"

#include <boost/foreach.hpp>

#include <iterator>
#include <algorithm>
#include <fstream>

static lg::log_domain log_ai("ai/general");
#define DBG_AI LOG_STREAM(debug, log_ai)
#define LOG_AI LOG_STREAM(info, log_ai)
#define WRN_AI LOG_STREAM(warn, log_ai)
#define ERR_AI LOG_STREAM(err, log_ai)

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

namespace ai {

typedef util::array<map_location,6> adjacent_tiles_array;

std::string monte_ai::describe_self() const
{
	return "[monte_ai]";
}


void monte_ai::new_turn()
{
}


config monte_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "monte_ai";
	return cfg;
}

void monte_ai::play_turn()
{
	//game_events::fire("ai turn");

        //First decide if we think we can win a fight. For each possible attack we could perform next, monte carlo it forward, and estimate PnL
        //If there is an attack that is in our favor, then perform the best of these. Otherwise consider defense.
        //Second consider all defensive positions we could take. Monte carlo score the front line.
        //If attacking is bad but better than defending then attack.

}


#ifdef _MSC_VER
#pragma warning(pop)
#endif


} //end of namespace ai

