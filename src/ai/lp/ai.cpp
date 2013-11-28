/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */

#include "ai.hpp"
#include "lp_solve.hpp"

#include "../actions.hpp"
#include "../game_info.hpp"
#include "../manager.hpp"

#include "../../attack_prediction.hpp"
#include "../../actions/attack.hpp"
//#include "../../dialogs.hpp"
//#include "../../game_events/pump.hpp"
#include "../../gamestatus.hpp"
#include "../../log.hpp"
#include "../../map_location.hpp"
//#include "../../mouse_handler_base.hpp"
#include "../../resources.hpp"
//#include "../../terrain_filter.hpp"
#include "../../unit_display.hpp"
#include "../../wml_exception.hpp"
#include "../../pathfind/pathfind.hpp"
//#include "../../unit_map.hpp"

#include <stdio.h>
//#include <boost/foreach.hpp>

//#include <queue>
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
// Implementation of AI's
//*************************************************************

lp_ai::lp_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg), ctk_lps() { }

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

        clock_t c0 = clock();
        std::map<const map_location, ctk_pod >::iterator k;

        map_location adjacent_tiles[6];
        std::pair<Itor,Itor> range;

        REAL current_opt = (REAL) -1000000;
        REAL this_opt;
        map_location best_target;
        std::map< const map_location, ctk_pod >::iterator best;

        fwd_ptr j;
       
        for (k = ctk_lps.begin(); k != ctk_lps.end(); ++k) {
            this_opt = k->second.first->get_obj();
            if (this_opt > current_opt) {
                best = k;
                current_opt = this_opt;
            }
        }
                
        j = best->second.first->begin();
        best_target = *(best->second.first->defender);
        DBG_AI << "New Best Moves List:" << std::endl;
        for(size_t n = 0; n != 6; ++n) {
            range = dstsrc.equal_range(adjacent_tiles[n]);
            while(range.first != range.second) {
                const map_location& dst = range.first->first;
                const map_location& src = range.first->second;         
                //y = xt, so divide by t which is in row[Ncol]. if this is > 0 then move.
                DBG_AI << "Value" << (REAL)(k->second.first->get_var(j)) << " | " <<  src << " -> " << dst << " \\> " << best_target; 

                if (best->second.first->var_gtr(j,.1)) {//((row[j++]/row[Ncol]) > .01) { 
                    execute_move_action(src, dst); //best_moves_list.push_back(std::make_pair<map_location, map_location> (src, dst));
                    DBG_AI << "**"; //src << " -> " << dst << " \\> " << i->get_location() << std::endl; 
                }//{execute_move_action(src, dst, false, true);}
                DBG_AI << std::endl;
                ++range.first;
                ++j;
            }
            DBG_AI << "End of list." << std::endl;
        }        
        clock_t c1 = clock();        
        double runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;
        DBG_AI << "Finish lp_ai::play_turn(), took "<< runtime_diff_ms << " ms." << std::endl;

}

void lp_ai::buildLPs()
{
        DBG_AI << "lp_ai::buildLPs();" <<std::endl;
#ifdef MCLP_FILEOUT
        char cstr[40];
        char file_name[20];
        int file_counter = -1; //damage lp is 0
#endif
        clock_t c0 = clock();

        int Ncol = 0; 

        ctk_lps.clear();
        boost::shared_ptr<ctkLP> current_target;
        //ctkLP current_target(map_location::null_location);
        bool haveTarget = false;
        dmg_lp = boost::shared_ptr<damageLP> (new damageLP());
        //dmg_lp = & dmg_lp_new;

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
                             current_target.reset(new ctkLP(&(i->get_location())));
                             current_target->set_obj_num_constant(-(REAL) i->hitpoints() * 100); // * 100 because CTH is an integer
                             current_target->set_obj_denom_constant((REAL) 1);

                             ctk_lps.insert(std::make_pair(i->get_location(), std::make_pair(boost::shared_ptr<ctkLP>(current_target), current_target->begin())));
                             haveTarget = true;
                         }
                         current_target->insert(range.first->second, range.first->first);
                         ++range.first;
                     }
                 }
            }
        }

        DBG_AI << "Making lps and pointers" << std::endl;

        //now make pointers to iterate for the ctk_lps.
        dmg_lp->make_lp();

        std::map<const map_location, ctk_pod >::iterator k;
        for(k = ctk_lps.begin(); k != ctk_lps.end(); ++k)
        {
            k->second.first->make_lp();
        }
        fwd_ptr j=dmg_lp->begin();

        DBG_AI << "Loading stats" << std::endl;

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {        
                 k = ctk_lps.find(i->get_location());
                 
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

#ifdef MCLP_FILEOUT
                         std::stringstream s1,s2,s3;
                         s1 << src; s2 << dst; s3 << i->get_location();
                         sprintf(cstr, "(%s->%s\\>%s)", s1.str().c_str(), s2.str().c_str(), s3.str().c_str());
                         dmg_lp->set_col_name(j, cstr);//lp_solve::set_col_name(lp, j, cstr);
                         k->second.first->set_col_name(k->second.second, cstr);
#endif

                         //const int chance_to_hit = un->second.defense_modifier(get_info().map,terrain);
                         //This code modified from attack::perform() in attack.cpp
                         /*  battle_context(const unit_map &units,
					               const map_location& attacker_loc, const map_location& defender_loc,
					               int attacker_weapon = -1, int defender_weapon = -1,
					               double aggression = 0.0, const combatant *prev_def = NULL,
					               const unit* attacker_ptr=NULL);
                              */
                         battle_context *bc_ = new battle_context(units_, dst, i->get_location(), -1, -1, 0.0, NULL, &(*un));
                         const battle_context_unit_stats a = bc_->get_attacker_stats();

                         //This code later in attack::perform() in attack.cpp
                         combatant attacker(bc_->get_attacker_stats());
                         combatant defender(bc_->get_defender_stats());
                         attacker.fight(defender,false);
 
                         dmg_lp->set_boolean(j);
                         dmg_lp->set_obj(j++, (static_cast<REAL>(i->hitpoints()) - defender.average_hp()) * i->cost() / i->max_hitpoints()); 

                         const REAL damage_expected = (static_cast<REAL>(a.damage)) * a.chance_to_hit * a.num_blows;
                         const REAL damage_variance = (static_cast<REAL>(a.damage)) * a.damage * a.chance_to_hit * a.num_blows;

                         k->second.first->set_boolean(k->second.second);
                         k->second.first->set_obj_num(k->second.second,damage_expected);
                         k->second.first->set_obj_denom(k->second.second++,damage_variance);

                         //value of an attack is the expected gold-adjusted damage inflicted
                         ++range.first;
                         delete(bc_);
                     }
                 }
            }
        }

        clock_t c1 = clock();        
        double runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;
        DBG_AI << "Finished creating LPs. Took " << runtime_diff_ms << " ms. " << std::endl;

        c0 = clock();

        dmg_lp->solve();
#ifdef MCLP_FILEOUT
                 sprintf(file_name, "temp%3u.lp", ++file_counter);
                 LOG_AI << "Here's my damageLP in file " << file_name << std::endl;
                 ret = dmg_lp->write_lp(file_name); //lp_solve::write_lp(lp, file_name);
#endif

        for (k = ctk_lps.begin(); k != ctk_lps.end(); ++k)
        {
            k->second.first->solve();
#ifdef MCLP_FILEOUT
                 sprintf(file_name, "temp%3u.lp", ++file_counter);
                 LOG_AI << "Here's my LP to attack " << (*(k->second.first->defender)) << " in file " << file_name << std::endl;
                 ret = k->second.first->write_lp(file_name); //lp_solve::write_lp(lp, file_name);
#endif
        }

        c1 = clock();        
        runtime_diff_ms = (c1 - c0) * 1000. / CLOCKS_PER_SEC;
        DBG_AI << "Finished solving LPs. Took " << runtime_diff_ms << " ms. " << std::endl;
}

// ======== Test ai's to visiualize LP output ===========

lp_1_ai::lp_1_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg) { }

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

#ifdef MCLP_DEBUG
        DBG_AI << "MCLP AI: FULL DEBUG" << std::endl;
#endif
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
                         DBG_AI << "LP_AI Column:" << ++Ncol << ":" << range.first->second << " -> " << range.first->first << " \\> " << i->get_location() << std::endl;
                         ++range.first;
                     }
                 }
            }
        }
        //Now we have the constraint matrix for our LP. Start with 0 rows and Ncol cols (variables)
        lp.make_lp();

        DBG_AI << "made LP" << std::endl;

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
                         lp.set_boolean(j);
                         lp.set_obj(j++, (static_cast<REAL>(i->hitpoints()) - defender.average_hp()) * i->cost() / i->max_hitpoints()); 
                         //value of an attack is the expected gold-adjusted damage inflicted.
                         ++range.first;

                         delete(bc_);
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

lp_2_ai::lp_2_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg) { }

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
        DBG_AI << "MCLP AI: FULL DEBUG" << std::endl;
#endif
#ifdef MCLP_FILEOUT
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
        boost::shared_ptr<ctkLP> lp; 

        std::pair<Itor,Itor> range;

        map_location l;
        std::pair<locItor, locItor> lrange;

        std::vector<std::pair<map_location, map_location> > best_moves_list;
        map_location best_target;
        REAL this_opt;
        REAL current_opt = -1000000; //this should be set to theoretical smallest real number.

        map_location adjacent_tiles[6];

        clock_t c0, c1;
        double runtime_diff_ms;

        for(unit_map::iterator i = units_.begin(); i != units_.end(); ++i) {
            if(current_team().is_enemy(i->side()) && !i->incapacitated()) {
                 lp.reset(new ctkLP(&(i->get_location())));
                 Ncol = 0;        
                 c0 = clock();
#ifdef MCLP_DEBUG
                 DBG_AI << "Considering attack on : " << i->get_location() << std::endl;
                 //DBG_AI << "Setting num constant... hit points =" << i->hitpoints() << std::endl;
#endif
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
                         lp->insert(range.first->second, range.first->first);
                         DBG_AI << "LP_AI Col:" << ++Ncol << "=" << range.first->second << " -> " << range.first->first << " attack " << i->get_location() << std::endl;
                         ++range.first;
                     }
                 }

                 if (Ncol > 0) 
                 {
#ifdef MCLP_DEBUG
                 DBG_AI << "Total Ncol: " << Ncol << std::endl;
#endif


                 lp->make_lp();

#ifdef MCLP_DEBUG
                 DBG_AI << "Adding Fractional Constraints. Ncol = " << Ncol << std::endl;
                 DBG_AI << "Setting num constant... hit points =" << i->hitpoints() << std::endl;
#endif

                 lp->set_obj_num_constant(-(REAL) i->hitpoints() * 100); // * 100 because CTH is an integer

#ifdef MCLP_DEBUG
                 DBG_AI << "Set denom constant..." << std::endl;
#endif
                 lp->set_obj_denom_constant((REAL) 1);

#ifdef MCLP_DEBUG
                 DBG_AI << "begin second iteration" << std::endl;
#endif
                 fwd_ptr j=lp->begin(); //columns numbered from 1
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
                         lp->set_col_name(j, cstr);//lp_solve::set_col_name(lp, j, cstr);
#endif
                         lp->set_boolean(j);//lp_solve::set_lowbo(lp, j, 0); //columns numbered from 1
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
 
                         const REAL damage_expected = (static_cast<REAL>(a.damage)) * a.chance_to_hit * a.num_blows;
                         const REAL damage_variance = (static_cast<REAL>(a.damage)) * a.damage * a.chance_to_hit * a.num_blows;

                         lp->set_obj_num(j,damage_expected);
                         lp->set_obj_denom(j,damage_variance);
                         ++j;
                         ++range.first;
                         delete(bc); //todo: put this back
                     }
                 }
                 assert(j == lp->end()); 

                 c1 = clock();
                 runtime_diff_ms = ((double)c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Took " << runtime_diff_ms << " ms to make LP.\n";
                 LOG_AI << "Solving...\n";

                 c0 = clock();

                 ret = lp->solve(); //lp_solve::solve(lp);

                 c1 = clock();
                 runtime_diff_ms = ((double)c1 - c0) * 1000. / CLOCKS_PER_SEC;

                 LOG_AI << "Done. Took " << runtime_diff_ms << " ms to solve.\n";
#ifdef MCLP_FILEOUT
                 sprintf(file_name, "temp%3u.lp", ++file_counter);
                 LOG_AI << "Here's my LP to attack " << i->get_location() << " in file " << file_name << std::endl;
                 ret = lp->write_lp(file_name); //lp_solve::write_lp(lp, file_name);
#endif
                 if(ret != LP_SOLVE_OPTIMAL)
                 {
                        DBG_AI << "**** NOT OPTIMAL: Code = " << ret << ":"<< lp_solve::SOLVE_CODE(ret) << "****" << std::endl;
                 }
#ifdef MCLP_DEBUG
                 DBG_AI << "Getting vars..." << std::endl;
#endif
                 this_opt = lp->get_obj(); //lp_solve::get_objective(lp);
#ifdef MCLP_DEBUG
                 DBG_AI << "Current Opt = " << current_opt << std::endl;
                 DBG_AI << "This Opt = " << this_opt << std::endl;
#endif
                 if( this_opt > current_opt ) { //now store the list of best moves and update opt and target
#ifdef MCLP_DEBUG
                      DBG_AI << "*** Better than best so far" << std::endl;
#endif
                      current_opt = this_opt;
                      best_moves_list.clear(); //best_moves_list = new std::vector<std::pair<map_location, map_location> >();

                      //ret = lp_solve::get_variables(lp, row); //row is already the right size
                      //assert(ret == LP_SOLVE_TRUE);

                      j = lp->begin();
                      best_target = i->get_location();
                      DBG_AI << "New Best Moves List:" << std::endl;
                      for(size_t n = 0; n != 6; ++n) {
                          range = dstsrc.equal_range(adjacent_tiles[n]);
                          while(range.first != range.second) {
                              const map_location& dst = range.first->first;
                              const map_location& src = range.first->second;         
                              //y = xt, so divide by t which is in row[Ncol]. if this is > 0 then move.
                              DBG_AI << "Value" << (REAL)(lp->get_var(j)) << " | " <<  src << " -> " << dst << " \\> " << i->get_location(); 

                              if (lp->var_gtr(j,.1)) {//((row[j++]/row[Ncol]) > .01) { 
                                  best_moves_list.push_back(std::make_pair<map_location, map_location> (src, dst));
                                  DBG_AI << "**"; //src << " -> " << dst << " \\> " << i->get_location() << std::endl; 
                              }//{execute_move_action(src, dst, false, true);}
                              DBG_AI << std::endl;
                              ++range.first;
                              ++j;
                          }
                      }
                      DBG_AI << "End of list." << std::endl;
                 } else //if opt better than best end
                 { DBG_AI << "Not the best. best_moves_list.size() = " << best_moves_list.size() << std::endl; }
                 } //if ncol > 0 end
            } //if end
        } // for end
        DBG_AI << "Making best moves: " << std::endl;

        std::pair<map_location, map_location> temp_pair;
        for (std::vector<std::pair<map_location, map_location> >::iterator j = best_moves_list.begin(); j != best_moves_list.end(); ++j)
        {
              DBG_AI << j->first << " -> " << j->second << " \\> " << best_target << std::endl;
              execute_move_action(j->first, j->second);
              execute_attack_action(j->second, best_target, -1);
        }
        DBG_AI << "Done." << std::endl;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif


} //end of namespace ai

