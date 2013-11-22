/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */

#include "ai.hpp"

#include "../actions.hpp"
#include "../manager.hpp"

#include "../../attack_prediction.hpp"
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

#include <time.h>
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

#ifndef REAL 
   #define REAL double
#endif

typedef struct _lprec      lprec;

namespace lp_solve
{
    extern "C"
    {
        lprec* make_lp(int, int);        
        unsigned char set_add_rowmode(lprec*, unsigned char);
        unsigned char add_constraint(lprec*,REAL*,int, REAL);
        unsigned char add_constraintex(lprec*,int,REAL*,int*, int, REAL);
        unsigned char set_binary(lprec*,int, unsigned char);
        unsigned char set_obj(lprec*,int,REAL);
        unsigned char set_lowbo(lprec*, int, REAL);
        void set_maxim(lprec*);
        void set_verbose(lprec*, int);
        int solve(lprec*);
        unsigned char get_variables(lprec*, REAL*);
        REAL get_objective(lprec*);
        void delete_lp(lprec*);

        unsigned char write_LP(lprec*, FILE*);
        unsigned char write_lp(lprec*, char*);

        //codes for ADD_CONSTRAINT
        int LE = 1;
        int GE = 2;
        int EQ = 3;

        //codes for SET_BINARY
        unsigned char TRUE = 1;
        unsigned char FALSE = 0;

        //codes for SET_VERBOSE
        int NEUTRAL = 0;
        int CRITICAL = 1;
        int SEVERE = 2;
        int IMPORTANT = 3;
        int NORMAL = 4;
        int DETAILED = 5;
        int FULL = 6;

        //codes for SOLVE
        int NOMEMORY = -2;
        int OPTIMAL = 0;
        int SUBOPTIMAL = 1;
        int INFEASIBLE = 2;
        int UNBOUNDED = 3;
        int DEGENERATE = 4;
        int NUMFAILURE = 5;
    }
}

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

lp_1_ai::lp_1_ai(readwrite_context &context, const config& /*cfg*/)
{
	init_readwrite_context_proxy(context);
}


std::string lp_1_ai::describe_self() const
{
	return "[lp_1_ai]";
}


void lp_1_ai::new_turn()
{
}

void lp_1_ai::switch_side(side_number side)
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
        clock_t c0 = clock();

        typedef move_map::const_iterator Itor;

        std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);

	unit_map& units_ = *resources::units;
        //gamemap& map_ = *resources::game_map;

        // for reference, LP_solve template file is here: http://lpsolve.sourceforge.net/5.5/formulate.htm
        lprec *lp;
        int Ncol = 0;
        int *col = NULL, j, ret;
        //unsigned char ret;
        REAL *row = NULL;

        //Each attacker destination "slot" becomes a row in LP since these are exclusive.
        std::multimap<map_location, int> *locMap = new std::multimap<map_location,int>();
        //Each unit can only attack once so this becomes a row in LP as well. Key will be unit.underlying_id();
        std::multimap<size_t, int> *unitMap = new std::multimap<size_t, int>();

        typedef std::multimap<map_location,int>::iterator locItor;
        typedef std::multimap<size_t,int>::iterator unitItor;

        std::pair<Itor,Itor> range;

        map_location adjacent_tiles[6];

        //i sis a unit iterator old info: i->first will be loc, and i->second will be unit, which is enemy
        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side())) {        
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         Ncol++;
                         locMap->insert(std::make_pair<map_location,int> (adjacent_tiles[n],Ncol));

                         const unit_map::const_iterator un = units_.find(range.first->second);
                         assert(un != units_.end());

                         unitMap->insert(std::make_pair<size_t,int> (un->underlying_id(), Ncol));

                         ++range.first;
                     }
                 }
            }
        }

        //Now we have the constraint matrix for our LP. Start with 0 rows and Ncol cols (variables)
        lp = lp_solve::make_lp(0,Ncol);
        assert(lp != NULL);

        //Going to add rows (constraints) one at a time.
        lp_solve::set_add_rowmode(lp, true);

        //Iterate over the "slots" and add a 1 in each column for an attack that goes through this slot
        map_location l;
        std::pair<locItor, locItor> lrange;
        size_t cnt;
        for (locItor i = locMap->begin(); i != locMap->end(); i = locMap->upper_bound(i->first))
        {
            l = i->first;
            cnt = locMap->count(l);

            j = 0;
            col = (int*) malloc(cnt * sizeof(*col));
            row = (REAL*) malloc(cnt * sizeof(*row));

            lrange = locMap->equal_range(l);

            //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
            while (lrange.first != lrange.second) {
                col[j] = lrange.first->second;
                row[j++] = (REAL) 1.0;
                ++lrange.first;
            }

            ret = lp_solve::add_constraintex(lp,j,row,col, lp_solve::LE, 1);
            assert(ret);

            free(col);
            free(row);
        }

        //Iterate over the "units" and add a 1 in each column for an attack that this unit could make
        std::pair<unitItor, unitItor> urange;
        for (unitItor i = unitMap->begin(); i != unitMap->end(); i = unitMap->upper_bound(i->first))
        {
            size_t u = i->first;
            cnt = unitMap->count(u);

            j = 0;
            col = (int*) malloc(cnt * sizeof(*col));
            row = (REAL*) malloc(cnt * sizeof(*row));

            urange = unitMap->equal_range(u);

            //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
            while (urange.first != urange.second) {
                col[j] = urange.first->second;
                row[j++] = (REAL) 1.0;
                ++urange.first;
            }

            ret = lp_solve::add_constraintex(lp,j,row,col, lp_solve::LE, 1);
            assert(ret);

            free(col);
            free(row);
        }

        lp_solve::set_add_rowmode(lp,lp_solve::FALSE);
        j=0;

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side())) {        
                 //location adjacent_tiles[6];
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         const map_location& dst = range.first->first;
                         const map_location& src = range.first->second;
                         //these are dst and src for the attacking unit

                         const unit_map::const_iterator un = units_.find(src);
                         assert(un != units_.end());

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
                              battle_context *bc_ = new battle_context(units_, dst, i->get_location(), -1, -1, 0.0, NULL, &(*un));

                              //This code later in attack::perform() in attack.cpp
                              combatant attacker(bc_->get_attacker_stats());
                              combatant defender(bc_->get_defender_stats());
                              attacker.fight(defender,false);
                              const REAL gold_inflicted = (static_cast<double>(i->hitpoints()) - defender.average_hp()) * i->cost() / i->max_hitpoints();
                         
                              lp_solve::set_binary(lp, j, lp_solve::TRUE); //set all variables to binary
                              lp_solve::set_obj(lp, j++, gold_inflicted);
                         }

                         ++range.first;
                     }
                 }
            }
        }

        lp_solve::set_maxim(lp);
        lp_solve::set_verbose(lp, lp_solve::IMPORTANT);

clock_t c1 = clock();
double runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;

        LOG_AI << "Took " << runtime_diff_ms << " ms to make LP.\n";
        //LOG_AI << "Here's my LP in file temp.lp:\n";
        //ret = lp_solve::write_lp(lp, "temp.lp");
        LOG_AI << "Solving...\n";

        c0 = clock();
// .../
        ret = lp_solve::solve(lp);

        c1 = clock();
        runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;

        LOG_AI << "Done. Took " << runtime_diff_ms << " ms to solve.\n";
        assert(ret == lp_solve::OPTIMAL);

/*  if(ret == 0) {
    // a solution is calculated, now lets get some results 

    // objective value 
    printf("Objective value: %f\n", get_objective(lp));

    // variable values */
    row = (REAL*) malloc(Ncol * sizeof(*row));
    ret = lp_solve::get_variables(lp, row);
    assert(ret == lp_solve::TRUE);
/*    for(j = 0; j < Ncol; j++)
      printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

    // we are done now 
  }
*/
        j = 0;
        for(unit_map::const_iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side())) {        
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         const map_location& dst = range.first->first;
                         const map_location& src = range.first->second;
                         //these are dst and src for the attacking unit

                         const unit_map::const_iterator un = units_.find(src);
                         assert(un != units_.end());

                         if (row[j++] > .01) {execute_move_action(src, dst, false, true);}
                         ++range.first;
                     }
                 }
            }
        }

  /* free allocated memory */
  if(row != NULL)
    free(row);
  if(col != NULL)
    free(col);

  if(lp != NULL) {
    /* clean up such that all used memory by lpsolve is freed */
    lp_solve::delete_lp(lp);
  }

}

// **********************************************************************************************************
//     END OF LP_1_AI
// **********************************************************************************************************
//
//
//
// **********************************************************************************************************
//     START OF LP_2_AI
// **********************************************************************************************************

lp_2_ai::lp_2_ai(readwrite_context &context, const config& /*cfg*/)
{
	init_readwrite_context_proxy(context);
}

std::string lp_2_ai::describe_self() const
{
	return "[lp_2_ai]";
}


void lp_2_ai::new_turn()
{
}

void lp_2_ai::switch_side(side_number side)
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
        typedef move_map::const_iterator Itor;
        typedef std::multimap<map_location,int>::iterator locItor;
        typedef std::multimap<size_t,int>::iterator unitItor;

        std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);

	unit_map& units_ = *resources::units;
        //gamemap& map_ = *resources::game_map;

        // for reference, LP_solve template file is here: http://lpsolve.sourceforge.net/5.5/formulate.htm
        lprec *lp;
        int Ncol = 0;
        int *col = NULL, j, ret;
        //unsigned char ret;
        REAL *row = NULL;

        int *shortcol = NULL;
        REAL *shortrow = NULL;

        size_t cnt;

        std::pair<Itor,Itor> range;

        map_location l;
        std::pair<locItor, locItor> lrange;

        size_t u;
        std::pair<unitItor, unitItor> urange;

        //Each attacker destination "slot" becomes a row in LP since these are exclusive.
        std::multimap<map_location, int> *locMap; // = new std::multimap<map_location,int>();
        //Each unit can only attack once so this becomes a row in LP as well. Key will be unit.underlying_id();
        std::multimap<size_t, int> *unitMap; //= new std::multimap<size_t, int>();

        std::vector<std::pair<map_location, map_location> > *best_moves_list = NULL;
        REAL current_opt = -1000000; //this should be theoretical smallest real number.

        map_location adjacent_tiles[6];

        clock_t c0, c1;
        double runtime_diff_ms;

        //i sis a unit iterator old info: i->first will be loc, and i->second will be unit, which is enemy
        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side())) {
                 locMap = new std::multimap<map_location, int>;
                 unitMap = new std::multimap<size_t, int>;
                 Ncol = 0;        
                 c0 = clock();


                 //get all ways to attack this unit, so we can set up our LP
                 //we solve a fractional LP, mean damage / variance damage. For well-conditioning we add one to the variance.
                 //Ncol will count number of cols in the fractional LP, but the transformation to a standard LP introduces 
                 //a variable "t" which will be Ncol+1. See wikipedia fractional linear programming.
                 //
                 // max c^T * x + \alpha / d^T x + \beta
                 // Ax \leq b
                 //
                 // is equivalent to
                 // 
                 // max c^T * y + \alpha t
                 // Ax \leq bt
                 // t \geq 0
                 // d^T y + \beta t = 1

                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         Ncol++;
                         locMap->insert(std::make_pair<map_location,int> (adjacent_tiles[n],Ncol));

                         const unit_map::const_iterator un = units_.find(range.first->second);
                         assert(un != units_.end());

                         unitMap->insert(std::make_pair<size_t,int> (un->underlying_id(), Ncol));

                         ++range.first;
                     }
                 }
    
                 //Now we have the constraint matrix for our LP. Start with 0 rows and Ncol cols (variables)
                 lp = lp_solve::make_lp(0,Ncol);
                 assert(lp != NULL);
        
                 //Going to add rows (constraints) one at a time.
                 lp_solve::set_add_rowmode(lp, true);
        
                 //Iterate over the "slots" and add a 1 in each column for an attack that goes through this slot
                 //map_location l;
                 //std::pair<locItor, locItor> lrange;
                 for (locItor k = locMap->begin(); k != locMap->end(); k = locMap->upper_bound(k->first))
                 {
                     l = k->first;
                     cnt = locMap->count(l);
        
                     j = 0;
                     col = (int*) malloc(cnt * sizeof(*col));
                     row = (REAL*) malloc(cnt * sizeof(*row));
         
                     lrange = locMap->equal_range(l);
        
                     //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
                     while (lrange.first != lrange.second) {
                         col[j] = lrange.first->second;
                         row[j++] = (REAL) 1.0;
                         ++lrange.first;
                     }
                     col[j] = Ncol+1;
                     row[j++] = (REAL) -1.0;
 
                     ret = lp_solve::add_constraintex(lp,j,row,col, lp_solve::LE, 0); //The constraint is Ax <= bt
                     assert(ret);
 
                     free(col);
                     free(row);
                 }
 
                 //Iterate over the "units" and add a 1 in each column for an attack that this unit could make
                 //size_t u;
                 //std::pair<unitItor, unitItor> urange;
                 for (unitItor k = unitMap->begin(); k != unitMap->end(); k = unitMap->upper_bound(k->first))
                 {
                     u = k->first;
                     cnt = unitMap->count(u) + 1;
 
                     j = 0;
                     col = (int*) malloc(cnt * sizeof(*col));
                     row = (REAL*) malloc(cnt * sizeof(*row));
 
                     urange = unitMap->equal_range(u);
 
                     //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
                     while (urange.first != urange.second) {
                         col[j] = urange.first->second;
                         row[j++] = (REAL) 1.0;
                         ++urange.first;
                     }
                     col[j] = Ncol+1;
                     row[j++] = (REAL) -1.0;
 
                     ret = lp_solve::add_constraintex(lp,j,row,col, lp_solve::LE, 0); //The constraint is Ax <= bt
                     assert(ret);
 
                     free(col);
                     free(row);
                 }

                 //Now add fractional constraints: t >= 0;
                 //row[0] = (REAL) 1.0;
                 //col[0] = Ncol + 1;
                 //lp_solve::add_constraintex(lp, 1, row, col, lp_solve::GE, 0);

                 lp_solve::set_lowbo(lp, Ncol+1, 0);

                 lp_solve::set_obj(lp, Ncol+1, -(REAL) i->hitpoints());

                 //Now add fractional constraints: d^t * y + \beta t = 1, and objective c^t y. 
                 //c is mean damage of an attack, d is variance, alpha should be negative of hitpoints in this model. 
                 
                 //We must also translate the boolean constraints x_i <= 1 to a constraint y_i <= t. 

                 row = (REAL*) malloc( (Ncol+1) * sizeof(*row));
                 //col = (int*) malloc(  (Ncol+1) * sizeof(*col)); //last col will just be col[j]=j so we can skip it.

                 shortrow = (REAL*) malloc( 2 * sizeof(*shortrow));
                 shortcol = (int*)  malloc( 2 * sizeof(*shortcol));

                 shortrow[0] = 1; //coeffs are +1,-1. variables are y_i and t.
                 shortrow[1] = -1;
                 shortcol[1] = Ncol+1;

                 j=0;

                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         const map_location& dst = range.first->first;
                         const map_location& src = range.first->second;
                         //these are dst and src for the attacking unit

                         const unit_map::const_iterator un = units_.find(src);
                         assert(un != units_.end());

                         //const int chance_to_hit = un->second.defense_modifier(get_info().map,terrain);

                         lp_solve::set_lowbo(lp, j, 0);
                         //This code modified from attack::perform() in attack.cpp
                         {
                              /*
				battle_context(const unit_map &units,
					               const map_location& attacker_loc, const map_location& defender_loc,
					               int attacker_weapon = -1, int defender_weapon = -1,
					               double aggression = 0.0, const combatant *prev_def = NULL,
					               const unit* attacker_ptr=NULL);
                              */
                              battle_context *bc = new battle_context(units_, dst, i->get_location(), -1, -1, 0.0, NULL, &(*un));

                              const battle_context_unit_stats a = bc->get_attacker_stats();

                              //const REAL expected_damage = a.damage * a.cth * a.num_blows;
                              //const REAL variance_damage = a.damage * a.damage * a.cth * a.num_blows;
                         
                              shortcol[0] = j;
                              lp_solve::add_constraintex(lp, 2, row, col, lp_solve::LE, 0); //This adds the y_i <= t constraint, corresponding to x_i <= 1

                              //making d^T y part of d^T * y + beta *t = 1
                              //col[j] = j;
                              row[j] = static_cast<REAL> (a.damage * a.damage * a.chance_to_hit * a.num_blows);

                              lp_solve::set_obj(lp, j++, static_cast<REAL> (a.damage * a.chance_to_hit * a.num_blows));
                         }

                         ++range.first;
                     }
                 }
                 //col[j] = j; //At this point j = Ncol, the t variable, and we chose beta = 1 for well-conditioning, although properly should be 0.
                 row[j] = 1; 

                 lp_solve::add_constraint(lp,row, lp_solve::EQ, 1);

                 lp_solve::set_add_rowmode(lp,lp_solve::FALSE);

                 lp_solve::set_maxim(lp);
                 lp_solve::set_verbose(lp, lp_solve::IMPORTANT);

                 c1 = clock();
                 runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Took " << runtime_diff_ms << " ms to make LP.\n";
                 //LOG_AI << "Here's my LP in file temp.lp:\n";
                 //ret = lp_solve::write_lp(lp, "temp.lp");
                 LOG_AI << "Solving...\n";

                 c0 = clock();
                  // ...
                 ret = lp_solve::solve(lp);

                 c1 = clock();
                 runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Done. Took " << runtime_diff_ms << " ms to solve.\n";
                 assert(ret == lp_solve::OPTIMAL);

                 if( lp_solve::get_objective(lp) > current_opt ) {

                  /*  if(ret == 0) {
                      // a solution is calculated, now lets get some results 

                      // objective value 
                      printf("Objective value: %f\n", get_objective(lp));

                      // variable values */
                      //row = (REAL*) malloc(Ncol * sizeof(*row));
                      ret = lp_solve::get_variables(lp, row);
                      assert(ret == lp_solve::TRUE);

                      best_moves_list = new std::vector<std::pair<map_location, map_location> >();
                  /*    for(j = 0; j < Ncol; j++)
                        printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

                      // we are done now 
                      }
                  */
                 //now sparsify the results vector, and store opt (also compute exact ctk value?)
                          j = 0;
                          for(size_t n = 0; n != 6; ++n) {
                              range = dstsrc.equal_range(adjacent_tiles[n]);
                              //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                              while(range.first != range.second) {
                                  const map_location& dst = range.first->first;
                                  const map_location& src = range.first->second;
                                  //these are dst and src for the attacking unit
         
                                  //const unit_map::const_iterator un = units_.find(src);
                                  //assert(un != units_.end());
         
                                  if (row[j++] > .01) { 
                                      const std::pair<map_location, map_location> temp(src, dst); 
                                      best_moves_list->push_back(temp); 
                                  }//{execute_move_action(src, dst, false, true);}
                                  ++range.first;
                              }
                          }
                      } //if end
                 /* free allocated memory */
                 if(row != NULL)
                   free(row);
                 if(col != NULL)
                   free(col);

                 if(shortrow != NULL)
                   free(shortrow);
                 if(shortcol != NULL)
                   free(shortcol);

                 if(lp != NULL) {
                   /* clean up such that all used memory by lpsolve is freed */
                   lp_solve::delete_lp(lp);
                 }
            } //if end
        } // for end

        std::pair<map_location, map_location> temp_pair;
        while (!best_moves_list->empty())
        {
              temp_pair = best_moves_list->back(); 
              best_moves_list->pop_back();
              execute_move_action(temp_pair.first, temp_pair.second, false, true);
        }
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif


} //end of namespace ai

