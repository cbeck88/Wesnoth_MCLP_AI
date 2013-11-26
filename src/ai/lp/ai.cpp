/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */

#include "ai.hpp"
//#include "lp_solve.hpp"

#include "../actions.hpp"
#include "../game_info.hpp"
#include "../manager.hpp"

#include "../../attack_prediction.hpp"
#include "../../actions/attack.hpp"
#include "../../dialogs.hpp"
#include "../../game_events/pump.hpp"
#include "../../gamestatus.hpp"
#include "../../log.hpp"
#include "../../map_location.hpp"
#include "../../mouse_handler_base.hpp"
#include "../../resources.hpp"
#include "../../terrain_filter.hpp"
#include "../../unit_display.hpp"
#include "../../wml_exception.hpp"
//#include "../../pathfind/pathfind.hpp"

#include <stdio.h>
#include <boost/foreach.hpp>

#include <queue>
#include <iterator>
#include <algorithm>

static lg::log_domain log_ai("ai/general");
//this in lp.cpp now 
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

typedef move_map::const_iterator Itor;
typedef std::multimap<map_location,int>::iterator locItor;

//*************************************************************
// Implementation of LP wrappers
//*************************************************************

void damageLP::insert( const map_location src, const map_location dst, map_location target)
{
    if (lp != NULL) {
       ERR_AI << "ERROR: Tried to insert to a damageLP after makelp." << std::endl;
       return;
    }
    col_ptr ptr = cols.insert(cols.end(),++Ncol);
    slotMap.insert(std::make_pair<const map_location,col_ptr> (dst, ptr));
    unitMap.insert(std::make_pair<const map_location,col_ptr> (src, ptr)); 
    //defenderMap.insert(std::make_pair<map_location,col_ptr> (target, ptr)); 
}

void ctkLP::insert(map_location src, map_location dst)
{
    if (lp != NULL) {
       ERR_AI << "ERROR: Tried to insert to a ctkLP after makelp." << std::endl;
       return;
    }
    col_ptr ptr = cols.insert(cols.end(),++Ncol);
    slotMap.insert(std::make_pair<const map_location,col_ptr> (dst, ptr));
    unitMap.insert(std::make_pair<const map_location,col_ptr> (src, ptr)); 
}

#define remove_X(X) remove_ ## X (const map_location loc)                                                           \
{                                                                                                                   \
    std::pair<std::multimap<const map_location, col_ptr>::iterator,std::multimap<const map_location, col_ptr>::iterator> range; \
    int temp;                                                                                                       \
    std::priority_queue<int> to_die;                                                                                \
    for (range = X ## Map.equal_range(loc); range.first != range.second; ++range.first)                             \
    {                                                                                                               \
        temp = *(range.second->second);                                                                             \
        assert(0 < temp);                                                                                           \
        assert(temp <= Ncol);                                                                                       \
        to_die.push(temp);                                                                                          \
    }                                                                                                               \
    while (!to_die.empty())                                                                                         \
    {                                                                                                               \
        lp->delete_col(to_die.top());                                                                               \
        to_die.pop();                                                                                               \
        Ncol--;                                                                                                     \
    }                                                                                                               \
    temp = 0;                                                                                                       \
    for (col_ptr ptr = cols.begin(); ptr != cols.end(); ++ptr)                                                      \
    {                                                                                                               \
        (*ptr)=++temp;                                                                                              \
    }                                                                                                               \
    assert(Ncol == temp);                                                                                           \
}
    //If the return of equal range were guaranteed to be sorted wrt value we could use this:                        
    //But according to standard we cannot so we throw in a heap first ><
    //
    //for (range = slotMap.equal_range(dst); range.second != range.first; --range.second)
    //{
    //    temp = *(range.second->second);
    //    assert(0 < temp);
    //    assert(temp <= Ncol);
    //    LP->delete_col(temp);
    //    Ncol--;
    //}

void damageLP::remove_X(slot)
void damageLP::remove_X(unit)
void ctkLP::remove_X(slot)
void ctkLP::remove_X(unit)

unsigned char damageLP::remove_col(fwd_ptr ptr)
{
    unsigned char ret;
    ret = lp->delete_col(*ptr);
    fwd_ptr temp = cols.erase(ptr);
    while (temp != cols.end())
    {
        (*temp)--;
        ++temp;
    }
    Ncol--;
    return ret;
}

unsigned char ctkLP::remove_col(fwd_ptr ptr)
{
    unsigned char ret;
    ret = lp->delete_col(*ptr);
    fwd_ptr temp = cols.erase(ptr);
    while (temp != cols.end())
    {
        (*temp)--;
        ++temp;
    }
    Ncol--;
    return ret;
}

void damageLP::make_lp()
{
    lp = new LP(Ncol);

    lp->rows_LE_1(&slotMap);
    //DBG_AI << "added slot constraints" << std::endl;
    lp->rows_LE_1(&unitMap);
    //DBG_AI << "added unit constraints" << std::endl;

    //bool_ptr = cols.begin();
}

void ctkLP::make_lp()
{
    lp = new FracLP(Ncol);

    lp->rows_LE_1(&slotMap);
    //DBG_AI << "added slot constraints" << std::endl;
    lp->rows_LE_1(&unitMap);
    //DBG_AI << "added unit constraints" << std::endl;

    //bool_ptr = cols.begin();
}

unsigned char damageLP::set_obj(fwd_ptr ptr, REAL r)
{
    //if (ptr == bool_ptr) { lp->set_boolean(*ptr); bool_ptr++; }
    return lp->set_obj(*ptr,r);
}

unsigned char ctkLP::set_obj_num(fwd_ptr ptr, REAL r)
{
    //if (ptr == bool_ptr) { lp->set_boolean(*ptr); bool_ptr++; }
    return lp->set_obj_num(*ptr,r);
}

unsigned char ctkLP::set_obj_denom(fwd_ptr ptr, REAL r)
{
    return lp->set_obj_denom(*ptr,r);
}

unsigned char ctkLP::set_obj_num_constant(REAL r)
{
    return lp->set_obj_num_constant(r);
}

unsigned char ctkLP::set_obj_denom_constant(REAL r)
{
    return lp->set_obj_denom_constant(r);
}

unsigned char damageLP::set_col_name(fwd_ptr ptr, char *str)
{
    return lp->set_col_name(*ptr,str);
}

unsigned char ctkLP::set_col_name(fwd_ptr ptr, char *str)
{
    return lp->set_col_name(*ptr,str);
}

unsigned char damageLP::set_boolean(fwd_ptr ptr)
{
    return lp->set_boolean(*ptr);
}
/*    if (bool_ptr != cols.end())
    {
        ERR_AI << "damageLP: Going make to make all vars boolean..." << std::endl;
        for (bool_ptr = cols.begin(); bool_ptr != cols.end(); bool_ptr++)
        {
            lp->set_boolean(*bool_ptr);
        }
    }*/
unsigned char damageLP::solve()
{
    return lp->solve();
}


unsigned char ctkLP::set_boolean(fwd_ptr ptr)
{
    return lp->set_boolean(*ptr);
}
/*    if (bool_ptr != cols.end())
    {
        ERR_AI << "ctkLP: Going make to make all vars boolean..." << std::endl;
        ERR_AI << "ctkLP: WARNING: Some denominator values could be indetermined..." << std::endl;
        for (bool_ptr = cols.begin(); bool_ptr != cols.end(); bool_ptr++)
        {
            lp->set_boolean(*bool_ptr);
        }
    }*/

unsigned char ctkLP::solve()
{
    return lp->solve();
}

REAL damageLP::get_obj()
{
    return lp->get_obj();
}

REAL ctkLP::get_obj()
{
    return lp->get_obj();
}

REAL damageLP::get_var(fwd_ptr ptr)
{
    return lp->get_var(*ptr);
}

REAL ctkLP::get_var(fwd_ptr ptr)
{
    return lp->get_var(*ptr);
}

unsigned char damageLP::write_lp(char * file)
{
    return lp->write_lp(file);
}

unsigned char ctkLP::write_lp(char * file)
{
    return lp->write_lp(file);
}


//*************************************************************
// END Implementation of LP wrappers
//*************************************************************

//*************************************************************
// Implementation of AI's
//*************************************************************


std::string lp_ai::describe_self() const
{
	return "[lp_ai]";
}


void lp_ai::new_turn()
{
        LOG_AI << "lp_ai: new turn" << std::endl;
        buildLPs();
}

void lp_ai::on_create()
{
        LOG_AI << "creating an lp_ai" << std::endl;
}

config lp_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_ai";
	return cfg;
}

void lp_ai::switch_side(side_number side)
{
        LOG_AI << "lp_ai new side: " << side << std::endl;
}


void lp_ai::play_turn()
{
	//game_events::fire("ai turn");
        LOG_AI << "lp_ai new turn" << std::endl;

        REAL current_opt = (REAL) -1000000;
        int* best_moves_list = NULL;
}

void lp_ai::buildLPs()
{
        clock_t c0 = clock();

        int Ncol = 0; 

#ifdef MCLP_DEBUG
        file_counter = 0;
#endif
        ctk_lps.clear();
        ctkLP current_target(map_location::null_location);
        bool haveTarget = false;
        damageLP dmg_lp_new;
        dmg_lp = & dmg_lp_new;

        std::map<map_location,pathfind::paths> possible_moves;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);

	unit_map& units_ = *resources::units;

        std::pair<Itor, Itor> range;

        map_location adjacent_tiles[6];

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {
                 haveTarget = false;        
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         DBG_AI << "LP_AI:" << Ncol << ":" << range.first->second << " -> " << range.first->first << " \\> " << i->get_location() << std::endl;
                         dmg_lp->insert(range.first->second,range.first->first,i->get_location());
                         if (!haveTarget) {
                             ctkLP current_target(i->get_location());
                             ctk_lps.insert(std::make_pair(i->get_location(), & current_target));
                             haveTarget = true;
                         }
                         current_target.insert(range.first->second, range.first->first);
                         ++range.first;
                     }
                 }
            }
        }
        clock_t c1 = clock();        
}

// ======== Test ai's to visiualize LP output ===========

//lp_1_ai::lp_1_ai(default_ai_context &context, const config& /*cfg*/)
//{
//        LOG_AI << "lp_1_constructed" << std::endl;
//}


std::string lp_1_ai::describe_self() const
{
	return "[lp_1_ai]";
}

void lp_1_ai::on_create()
{
        LOG_AI << "creating an lp_1_ai" << std::endl;
}

void lp_1_ai::new_turn()
{
        LOG_AI << "lp_1_ai new turn" << std::endl;
}

void lp_1_ai::switch_side(side_number side)
{
        LOG_AI << "lp_1_ai new side: " << side << std::endl;
}

config lp_1_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_1_ai";
	return cfg;
}

void lp_1_ai::play_turn()
{
        LOG_AI << "lp_1_ai play_turn()" << std::endl;

	//game_events::fire("ai turn");
        clock_t c0 = clock();

        std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);

	unit_map& units_ = *resources::units;

        // for reference, LP_solve template file is here: http://lpsolve.sourceforge.net/5.5/formulate.htm

        int Ncol = 0, ret;
        damageLP lp;

        std::pair<Itor,Itor> range;

        map_location adjacent_tiles[6];

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {        
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         //columns numbered from 1 in lib lp_solve
                         lp.insert(range.first->second, range.first->first, i->get_location());
                         //slotMap->insert(std::make_pair<map_location,int> (range.first->first,++Ncol));
                         //unitMap->insert(std::make_pair<map_location,int> (range.first->second, Ncol)); 
                         DBG_AI << "LP_AI Column:" << ++Ncol << ":" << range.first->second << " -> " << range.first->first << " \\> " << i->get_location() << std::endl;
                         ++range.first;
                     }
                 }
            }
        }
        //Now we have the constraint matrix for our LP. Start with 0 rows and Ncol cols (variables)
        lp.make_lp();

        DBG_AI << "made LP" << std::endl;

//        int j=0;
        fwd_ptr j=lp.begin();
        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {        
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
                         /*  battle_context(const unit_map &units,
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
                         //lp_solve::set_binary(lp, ++j, LP_SOLVE_TRUE); //set all variables to binary. columns numbered from 1!
                         lp.set_boolean(j);
                         lp.set_obj(j++, (static_cast<REAL>(i->hitpoints()) - defender.average_hp()) * i->cost() / i->max_hitpoints()); 
                         //value of an attack is the expected gold-adjusted damage inflicted.
                         ++range.first;
                     }
                 }
            }
        }

        clock_t c1 = clock();
        double runtime_diff_ms = ((double) c1 - c0) * 1000. / CLOCKS_PER_SEC;

        LOG_AI << "Took " << runtime_diff_ms << " ms to make LP.\n";
        //LOG_AI << "Here's my LP in file temp.lp:\n";
        //ret = lp_solve::write_lp(lp, "temp.lp");
        LOG_AI << "Solving...\n";

        c0 = clock();
// .../
        ret = lp.solve();

        c1 = clock();
        runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;

        LOG_AI << "Done. Took " << runtime_diff_ms << " ms to solve.\n";
        assert(ret == LP_SOLVE_OPTIMAL);

        j = lp.begin();
        for(unit_map::const_iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {        
                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     //adjacent_tiles[n] is the attacker dest hex, i->first is the defender hex
                     while(range.first != range.second) {
                         if (lp.get_var(j++) > .01) {
                            execute_move_action(range.first->second, range.first->first); //dst = range.first->first, src = range.first->second, hence name dstsrc...
                         } 
                         ++range.first;
                     }
                 }
            }
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

//lp_2_ai::lp_2_ai(default_ai_context &context, const config& /*cfg*/) : ai_composite(context, config)
//{
//        LOG_AI << "lp_2_constructed" << std::endl;
//	init_default_ai_context_proxy(context);
//}

std::string lp_2_ai::describe_self() const
{
	return "[lp_2_ai]";
}

void lp_2_ai::on_create()
{
        LOG_AI << "creating an lp_2_ai" << std::endl;
}

void lp_2_ai::new_turn()
{
        LOG_AI << "lp_2_ai new turn" << std::endl;
}

void lp_2_ai::switch_side(side_number side)
{
        LOG_AI << "lp_2_ai new side: " << side << std::endl;
}

config lp_2_ai::to_config() const
{
	config cfg;
	cfg["ai_algorithm"]= "lp_2_ai";
	return cfg;
}

void lp_2_ai::play_turn()
{
        LOG_AI << "lp_2_ai play_turn()" << std::endl;

#ifdef MCLP_DEBUG
        char cstr[40];
        char file_name[20];
        int file_counter = 0;
#endif
	//game_events::fire("ai turn");

        std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;
        calculate_possible_moves(possible_moves,srcdst,dstsrc,false);

	unit_map& units_ = *resources::units;
        //gamemap& map_ = *resources::game_map;

        int Ncol = 0, ret;
        ctkLP lp(map_location::null_location);
        // for reference, LP_solve template file is here: http://lpsolve.sourceforge.net/5.5/formulate.htm
        //lprec *lp;
        //int *col = NULL, j, ret;
        //unsigned char ret;
        //REAL *row = NULL;

        //int *shortcol = NULL;
        //REAL *shortrow = NULL;

        //size_t cnt;

        std::pair<Itor,Itor> range;

        map_location l;
        std::pair<locItor, locItor> lrange;

        //Each attacker destination "slot", and each attacker, becomes a row in LP since each unit and slot can be used only once.
        //std::multimap<map_location, int> *slotMap; // = new std::multimap<map_location,int>();
        //std::multimap<map_location, int> *unitMap; //= new std::multimap<size_t, int>();

        std::vector<std::pair<map_location, map_location> > *best_moves_list = NULL;
        map_location best_target;
        REAL this_opt;
        REAL current_opt = -1000000; //this should be set to theoretical smallest real number.

        map_location adjacent_tiles[6];

        clock_t c0, c1;
        double runtime_diff_ms;

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {
                 ctkLP lp(i->get_location());
                 //slotMap = new std::multimap<map_location, int>;
                 //unitMap = new std::multimap<map_location, int>;
                 Ncol = 0;        
                 c0 = clock();

                 DBG_AI << "Considering attack on : " << i->get_location() << std::endl;

                 //get all ways to attack this unit, so we can set up our LP
                 //we solve a fractional LP, mean damage / variance damage. For well-conditioning we add one to the variance.
                 //
                 // max c^T * x + \alpha / d^T x + \beta
                 // Ax \leq b
                 //
                 // Our program will have a column (corresponds to variable x_i) for each possible defender, slot and attacker (and weapon choice).
                 //
                 // The basic constraints Ax \leq b will say, each attacker used at most once, each slot used at most once. 
                 // We will set c^T_i = Expectation of damage to defender of attack @ column i.
                 //             d^T_i = Variance    of damage to defender ...
                 //             alpha = - Hitpoints of defender
                 //             beta  = 1
                 //
                 // Phi(Objective_Values) is approximately equal to the CTK, where Phi is standard gaussian cdf.

                 get_adjacent_tiles(i->get_location(),adjacent_tiles);
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     while(range.first != range.second) {
                         //slotMap->insert(std::make_pair<map_location,int> (range.first->first,++Ncol)); //columns numbered from 1 in lib lp_solve
                         //unitMap->insert(std::make_pair<map_location,int> (range.first->second, Ncol));
                         lp.insert(range.first->second, range.first->first);
                         DBG_AI << "LP_AI Col:" << ++Ncol << "=" << range.first->second << " -> " << range.first->first << " attack " << i->get_location() << std::endl;
                         ++range.first;
                     }
                 }

                 if (Ncol > 0) 
                 {
                 DBG_AI << "Total Ncol: " << Ncol << std::endl;

                 lp.make_lp();

/*                 lp = lp_solve::make_lp(0,Ncol+1); //Start with 0 rows and Ncol+1 cols (variables), since we have t in addition.
                 assert(lp != NULL); //todo: case out on errors
        
                 DBG_AI << "Made LP" << std::endl;
                 lp_solve::set_add_rowmode(lp, true); //Going to add rows (constraints) one at a time.
        
                 DBG_AI << "Adding Slot Constraints" << std::endl;
                 //Iterate over the "slots" and add a 1 in each column for an attack that goes through this slot
                 for (locItor k = slotMap->begin(); k != slotMap->end(); k = slotMap->upper_bound(k->first))
                 {
                     l = k->first;
                     cnt = slotMap->count(l);  
                     //DBG_AI << cnt << " units can attack from slot " << l << std::endl;

                     if (cnt > 0) {
                         cnt++; // +1, need to include a spot for t variable.
        
                         j = 0;
                         col = (int*)  malloc(cnt * sizeof(*col));
                         row = (REAL*) malloc(cnt * sizeof(*row));
         
                         lrange = slotMap->equal_range(l);
        
                         //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
                         while (lrange.first != lrange.second) {
                             col[j] = lrange.first->second;
                             row[j++] = (REAL) 1.0;
                             ++lrange.first;
                         }
                         col[j] = Ncol+1;
                         row[j++] = (REAL) -1.0;
 
                         ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 0); //The constraint is Ax <= bt
                         assert(ret);

                         free(col);
                         free(row);
                     }
                 }
 
                 DBG_AI << "Adding Unit Constraints" << std::endl;
                 //Iterate over the "units" and add a 1 in each column for an attack that this unit could make
                 for (locItor k = unitMap->begin(); k != unitMap->end(); k = unitMap->upper_bound(k->first))
                 {
                     l = k->first;
                     cnt = unitMap->count(l);

                     if (cnt > 0) {
                         cnt++; //+1, need to include a spot for t variable.
 
                         j = 0;
                         col = (int*) malloc(cnt * sizeof(*col));
                         row = (REAL*) malloc(cnt * sizeof(*row));
 
                         lrange = unitMap->equal_range(l);
 
                         //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
                         while (lrange.first != lrange.second) {
                             col[j] = lrange.first->second;
                             row[j++] = (REAL) 1.0;
                             ++lrange.first;
                         }
                         col[j] = Ncol+1;
                         row[j++] = (REAL) -1.0;
 
                         ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 0); //The constraint is Ax <= bt
                         assert(ret);
 
                         free(col);
                         free(row);
                     }
                 }*/
                 DBG_AI << "Adding Fractional Constraints. Ncol = " << Ncol << std::endl;

                 lp.set_obj_num_constant(-(REAL) i->hitpoints() * 100); // * 100 because CTH is an integer
                 lp.set_obj_denom_constant((REAL) 1);
/*
#ifdef MCLP_DEBUG
                 lp_solve::set_col_name(lp, Ncol+1, (char *)"t");
#endif
                 //Now add fractional constraint: t >= 0;
                 lp_solve::set_lowbo(lp, Ncol+1, 0);

                 lp_solve::set_obj(lp, Ncol+1, -(REAL) i->hitpoints() * 100); // * 100 because CTH is an integer

                 //Now add fractional constraints: d^t * y + \beta t = 1, and objective c^t y. 
                 //c is mean damage of an attack, d is variance, alpha should be negative of hitpoints in this model. 
                 
                 //We must also translate the boolean constraints x_i <= 1 to a constraint y_i <= t. 
        
                 row = (REAL*) malloc( (Ncol+1) * sizeof(*row));
                 col = NULL; //col = (int*) malloc(  (Ncol+1) * sizeof(*col)); //last col will just be col[j]=j+1 so we can skip it.

                 shortrow = (REAL*) malloc( 2 * sizeof(*shortrow));
                 shortcol = (int*)  malloc( 2 * sizeof(*shortcol));

                 shortrow[0] = 1; //coeffs are +1,-1. variables are y_i and t.
                 shortrow[1] = -1;
                 shortcol[1] = Ncol+1;*/

                 fwd_ptr j=lp.begin(); //columns numbered from 1
                 for(size_t n = 0; n != 6; ++n) {
                     range = dstsrc.equal_range(adjacent_tiles[n]);
                     while(range.first != range.second) {
                         const map_location& dst = range.first->first;
                         const map_location& src = range.first->second;

                         const unit_map::const_iterator un = units_.find(src);
                         assert(un != units_.end());
                         //const int chance_to_hit = un->second.defense_modifier(get_info().map,terrain);
#ifdef MCLP_DEBUG
                         std::stringstream s1,s2,s3;
                         s1 << src; s2 << dst; s3 << i->get_location();
                         sprintf(cstr, "(%s->%s\\>%s)", s1.str().c_str(), s2.str().c_str(), s3.str().c_str());
                         lp.set_col_name(j, cstr);//lp_solve::set_col_name(lp, j, cstr);
#endif
                         lp.set_boolean(j);//lp_solve::set_lowbo(lp, j, 0); //columns numbered from 1
                         //This code modified from attack::perform() in attack.cpp
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
                              //shortcol[0] = j;
                              //lp_solve::add_constraintex(lp, 2, shortrow, shortcol, LP_SOLVE_LE, 0); //This adds the y_i <= t constraint, corresponding to x_i <= 1

                              //making d^T y part of d^T * y + beta *t = 1
                              //col[j] = j+1;
                              //row[j] = static_cast<REAL> (a.damage * a.damage * a.chance_to_hit * a.num_blows);
 
                         lp.set_obj_num(j,static_cast<REAL> (a.damage * a.chance_to_hit * a.num_blows));
                         lp.set_obj_denom(j++,static_cast<REAL> (a.damage * a.damage * a.chance_to_hit * a.num_blows));
                              //lp_solve::set_obj(lp, j++, static_cast<REAL> (a.damage * a.chance_to_hit * a.num_blows));
                         ++range.first;
                     }
                 }
                 assert(j == lp.end()); 

                 c1 = clock();
                 runtime_diff_ms = ((double)c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Took " << runtime_diff_ms << " ms to make LP.\n";
#ifdef MCLP_DEBUG
                 sprintf(file_name, "temp%3u.lp", ++file_counter);
                 LOG_AI << "Here's my LP to attack " << i->get_location() << " in file " << file_name << std::endl;
                 ret = lp.write_lp(file_name); //lp_solve::write_lp(lp, file_name);
#endif
                 LOG_AI << "Solving...\n";

                 c0 = clock();

                 ret = lp.solve(); //lp_solve::solve(lp);

                 c1 = clock();
                 runtime_diff_ms = ((double)c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Done. Took " << runtime_diff_ms << " ms to solve.\n";
                 if(ret != LP_SOLVE_OPTIMAL)
                 {
                        DBG_AI << "**** NOT OPTIMAL ****" << std::endl;
                        //DBG_AI << "Here's my LP in file temp.lp:\n";
                        //ret = lp_solve::write_lp(lp, "temp.lp");
                        //DBG_AI << "Here's my data structures:\n" << "Slot Map:\n" << slotMap << std::endl << "Unit Map:\n" << unitMap << std::endl;
                 }
                 this_opt = lp.get_obj(); //lp_solve::get_objective(lp);
#ifdef MCLP_DEBUG
                 DBG_AI << "Current Opt = " << current_opt << std::endl;
                 DBG_AI << "This Opt = " << this_opt << std::endl;
#endif
                 if( this_opt > current_opt ) { //now store the list of best moves and update opt and target
                      DBG_AI << "*** Better than best so far" << std::endl;

                      current_opt = this_opt;
                      best_moves_list->clear(); //best_moves_list = new std::vector<std::pair<map_location, map_location> >();

                      //ret = lp_solve::get_variables(lp, row); //row is already the right size
                      //assert(ret == LP_SOLVE_TRUE);

                      j = lp.begin();
                      best_target = i->get_location();
                      DBG_AI << "New Best Moves List:" << std::endl;
                      for(size_t n = 0; n != 6; ++n) {
                          range = dstsrc.equal_range(adjacent_tiles[n]);
                          while(range.first != range.second) {
                              const map_location& dst = range.first->first;
                              const map_location& src = range.first->second;         
                              //y = xt, so divide by t which is in row[Ncol]. if this is > 0 then move.
                              if (lp.get_var(j)>.01) {//((row[j++]/row[Ncol]) > .01) { 
                                  best_moves_list->push_back(std::make_pair<map_location, map_location> (src, dst));
                                  DBG_AI << src << " -> " << dst << " \\> " << i->get_location() << std::endl; 
                              }//{execute_move_action(src, dst, false, true);}
                              ++range.first;
                          }
                      }
                      DBG_AI << "End of list." << std::endl;
                 } else //if opt better than best end
                 { DBG_AI << "Not the best" << std::endl; }
                 } //if ncol > 0 end
            } //if end
        } // for end
        DBG_AI << "Making best moves: " << std::endl;

        std::pair<map_location, map_location> temp_pair;
        while (!best_moves_list->empty())
        {
              temp_pair = best_moves_list->back(); 
              best_moves_list->pop_back();
              DBG_AI << temp_pair.first << " -> " << temp_pair.second << " \\> " << best_target << std::endl;
              execute_move_action(temp_pair.first, temp_pair.second);
              execute_attack_action(temp_pair.second, best_target, -1);
        }
        DBG_AI << "Done." << std::endl;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif


} //end of namespace ai

