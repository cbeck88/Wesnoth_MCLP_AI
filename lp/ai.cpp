/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */

#include "ai.hpp"

#include "../actions.hpp"
#include "../manager.hpp"

#include "../../actions/attack.hpp"
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
#include "lp_lib.h"

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

/*
std::string lp_ai::describe_self() const
{
	return "[lp_ai]";
}


void lp_ai::new_turn()
{
}


config lp_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_ai";
	return cfg;
}

void lp_ai::play_turn()
{
	//game_events::fire("ai turn");
}
*/
// ======== Test ai's to visiualize LP output ===========

std::string lp_1_ai::describe_self() const
{
	return "[lp_1_ai]";
}


void lp_1_ai::new_turn()
{
}


config lp_1_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_1_ai";
	return cfg;
}

void lp_1_ai::play_turn()
{
	//game_events::fire("ai turn");

        typedef move_map::const_iterator Itor;

        std::map<location,paths> possible_moves;
        move_map srcdst, dstsrc;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);


        // for reference, LP_solve template file is here: http://lpsolve.sourceforge.net/5.5/formulate.htm
        lprec *lp;
        int Ncol = 0;
        int *col, j, ret;
        REAL *row;

        //Each attacker destination "slot" becomes a row in LP since these are exclusive.
        std::multimap<location, int> locMap;
        //Each unit can only attack once so this becomes a row in LP as well.
        std::multimap<unit, int> unitMap;

        typedef std::multimap<location,int>::iterator locItor;
        typedef std::multimap<unit,int>::iterator unitItor;

        //i->first will be loc, and i->second will be unit, which is enemy
        for(unit_map::const_iterator i = get_info().units.begin(); i != get_info().units.end(); ++i) {
            if(current_team().is_enemy(i->second.side()) {        
                 location adjacent_tiles[6];
                 get_adjacent_tiles(i->first,adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     std::pair<Itor,Itor> range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         Ncol++;
                         locMap.emplace(adjacent_tiles[n],Ncol);

                         const unit_map::const_iterator un = get_info().units.find(src);
                         assert(un != get_info().units.end());

                         unitMap.emplace(un->second, Ncol);

                         ++range.first;
                     }
                 }
            }
        }

        //Now we have the constraint matrix for our LP. Start with 0 rows and Ncol cols (variables)
        lp = make_lp(0,Ncol);
        assert(lp != NULL);

        //Going to add rows (constraints) one at a time.
        set_add_rowmode(lp, true);

        //Iterate over the "slots" and add a 1 in each column for an attack that goes through this slot
        location l;
        std::pair<locItor, locItor> lrange;
        size_t cnt;
        for (locItor i = locMap.begin(); i != locMap.end(); i = locMap.upper_bound(i->first))
        {
            l = i->first;
            cnt = locMap.count(l);

            j = 0;
            col = (int*) malloc(cnt * size_of(*col));
            row = (REAL*) malloc(cnt * size_of(*row));

            lrange = locMap.equal_range(l);

            //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
            while (lrange.first != lrange.second) {
                col[j] = lrange.first->second;
                row[j++] = (REAL) 1.0;
                ++lrange.first;
            }

            ret = add_constraintex(lp,j,row,col, LE, 1);
            assert(ret);

            free(col);
            free(free);
        }

        //Iterate over the "units" and add a 1 in each column for an attack that this unit could make
        unit u;
        std::pair<unitItor, unitItor> urange;
        size_t cnt;
        for (unitItor i = unitMap.begin(); i != unitMap.end(); i = unitMap.upper_bound(i->first))
        {
            u = i->first;
            cnt = unitMap.count(u);

            j = 0;
            col = (int*) malloc(cnt * size_of(*col));
            row = (REAL*) malloc(cnt * size_of(*row));

            urange = unitMap.equal_range(u);

            //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
            while (urange.first != urange.second) {
                col[j] = urange.first->second;
                row[j++] = (REAL) 1.0;
                ++urange.first;
            }

            ret = add_constraintex(lp,j,row,col, LE, 1);
            assert(ret);

            free(col);
            free(row);
        }

        j=0;

        for(unit_map::const_iterator i = get_info().units.begin(); i != get_info().units.end(); ++i) {
            if(current_team().is_enemy(i->second.side()) {        
                 location adjacent_tiles[6];
                 get_adjacent_tiles(i->first,adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     std::pair<Itor,Itor> range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         const location& dst = range.first->first;
                         const location& src = range.first->second;
                         //these are dst and src for the attacking unit

                         const unit_map::const_iterator un = get_info().units.find(src);
                         assert(un != get_info().units.end());

                         //const int chance_to_hit = un->second.defense_modifier(get_info().map,terrain);

                         //This code modified from attack::perform() in attack.cpp
                         {
                              /*
				battle_context(const unit_map &units,
					               const map_location& attacker_loc, const map_location& defender_loc,
					               int attacker_weapon = -1, int defender_weapon = -1,
					               double aggression = 0.0, const combatant *prev_def = NULL,
					               const unit* attacker_ptr=NULL);
                              */
                              battle_context bc_ = new battle_context(get_info().units, dst, i->first, -1, -1, 0.0, NULL, &un->second);

                              //This code later in attack::perform() in attack.cpp
                              combatant attacker(bc_->get_attacker_stats());
                              combatant defender(bc_->get_defender_stats());
                              attacker.fight(defender,false);
                              const double gold_inflicted = (static_cast<double>(i->second.hitpoints()) - defender.average_hp()) * i->second.cost() / i->second.max_hitpoints();
                              
                         }

                         set_binary(lp, j, TRUE); //set all variables to binary
                         set_obj(lp, j++, (REAL) gold_inflicted);

                         ++range.first;
                     }
                 }
            }
        }

        set_maxim(lp);
        set_verbose(lp, IMPORTANT);
        ret = solve(lp);
        assert(ret == OPTIMAL);

/*  if(ret == 0) {
    // a solution is calculated, now lets get some results 

    // objective value 
    printf("Objective value: %f\n", get_objective(lp));

    // variable values */
    get_variables(lp, row);
/*    for(j = 0; j < Ncol; j++)
      printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

    // we are done now 
  }
*/
        j = 0;
        for(unit_map::const_iterator i = get_info().units.begin(); i != get_info().units.end(); ++i) {
            if(current_team().is_enemy(i->second.side()) {        
                 location adjacent_tiles[6];
                 get_adjacent_tiles(i->first,adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     std::pair<Itor,Itor> range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         const location& dst = range.first->first;
                         const location& src = range.first->second;
                         //these are dst and src for the attacking unit

                         const unit_map::const_iterator un = get_info().units.find(src);
                         assert(un != get_info().units.end());

                         if (row[j++] > .01) {execute_move_action(src, dst, false, true);}
                         ++range.first;
                     }
                 }
            }
        }

  /* free allocated memory */
  if(row != NULL)
    free(row);
  if(colno != NULL)
    free(col);

  if(lp != NULL) {
    /* clean up such that all used memory by lpsolve is freed */
    delete_lp(lp);
  }

}

/*
std::string lp_2_ai::describe_self() const
{
	return "[lp_2_ai]";
}


void lp_2_ai::new_turn()
{
}


config lp_2_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_2_ai";
	return cfg;
}

void lp_2_ai::play_turn()
{
	//game_events::fire("ai turn");
}
*/

#ifdef _MSC_VER
#pragma warning(pop)
#endif


} //end of namespace ai

