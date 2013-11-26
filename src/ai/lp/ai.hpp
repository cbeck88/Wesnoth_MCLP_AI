/*
   Copyright (C) 2003 - 2013 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/** @file */

#ifndef AI_LP_AI_HPP_INCLUDED
#define AI_LP_AI_HPP_INCLUDED

//Uncomment this to get full debugging output
//#define MCLP_DEBUG

#ifdef MCLP_DEBUG
#define LP_SOLVE_LOG_MODE LP_SOLVE_FULL
#else
#define LP_SOLVE_LOG_MODE LP_SOLVE_IMPORTANT
#endif

#include "lp.hpp"
#include "../contexts.hpp"
#include "../interface.hpp"
#include "../composite/ai.hpp"

#include <time.h>
#include <list>

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif


namespace pathfind {

struct plain_route;

} // of namespace pathfind

/** Interface for a set of constraints to boolean variables, with rows indexed by locations */

namespace ai {

typedef std::list<int>::iterator col_ptr;

//why no forward iterator
typedef std::list<int>::iterator fwd_ptr;

/** A class that manages an LP which estimates damage potential for the army. **/
//You must iterate make all insertions before calling make_lp.
//You must then iterate a forward pointer and set_obj to make all variables boolean.
//If you fail to do this the class will iterate again before you solve to make sure this happens.
//Best not to delete anything until after all vars have been made boolean.

class damageLP {
public:
    damageLP():lp(NULL),slotMap(), unitMap(), Ncol(0),cols(){} //defenderMap()
    //~damageLP() { if (lp != NULL) { lp_solve:delete_lp(lp); }

    void insert( map_location src, map_location dst, map_location def);
    void make_lp();
    void remove_slot(map_location dst);
    void remove_unit(map_location unit);

    fwd_ptr begin() {return cols.begin(); }
    fwd_ptr end() {return cols.end(); }
    
    unsigned char remove_col( fwd_ptr );

    unsigned char set_obj (fwd_ptr, REAL );
    unsigned char set_col_name (fwd_ptr, char *);

    unsigned char solve();

    REAL get_obj();
    REAL get_var(fwd_ptr);

private:
    LP *lp;

    //Each attacker destination "slot", and each attacker, becomes a row in LP since each unit and slot can be used only once.
    std::multimap<const map_location, col_ptr> slotMap; 
    std::multimap<const map_location, col_ptr> unitMap;
    //std::multimap<map_location, col_ptr> defenderMap; // No use atm

    int Ncol;

    //Rather than store ints for columns, store pointers to this list so we can delete easily.
    std::list<int> cols;

    fwd_ptr bool_ptr;
};

/** A class that manages an LP which estimates opt ctk for a target. **/
//You must iterate make all insertions before calling make_lp.
//You must then iterate a forward pointer and set_obj to make all variables boolean.
//If you fail to do this the class will iterate again before you solve to make sure this happens.
//Best not to remove anything until after all vars have been made boolean.

class ctkLP {
public:
    ctkLP(map_location ml):lp(NULL),slotMap(), unitMap(),defender(ml),Ncol(0),cols() {}
    //~ctkLP() { if (lp != NULL) { lp_solve::delete_lp(lp); }

    void insert( map_location src, map_location dst);
    void make_lp();
    void remove_slot( map_location dst);
    void remove_unit( map_location unit); 

    fwd_ptr begin() { return cols.begin(); }
    fwd_ptr end() { return cols.end(); }

    unsigned char remove_col( fwd_ptr );

    unsigned char set_obj_num(fwd_ptr ptr, REAL );
    unsigned char set_obj_denom(fwd_ptr ptr, REAL );
    unsigned char set_obj_num_constant(REAL );
    unsigned char set_obj_denom_constant(REAL );
    unsigned char set_col_name (fwd_ptr, char *);

    unsigned char solve();

    REAL get_obj();
    REAL get_var(fwd_ptr);
    
private:
    FracLP *lp;

    //Each attacker destination "slot", and each attacker, becomes a row in LP since each unit and slot can be used only once.
    std::multimap<const map_location, col_ptr> slotMap;
    std::multimap<const map_location, col_ptr> unitMap;
    map_location defender;

    int Ncol;

    //Rather than store ints for columns, store pointers to this forward list so we can delete easily.
    std::list<int> cols;

    fwd_ptr bool_ptr;
};

/** An ai that uses LP heuristics to try to find the most efficiently aggressive move. */

class lp_ai : public ai_composite {
public:
        lp_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg), temp_lp(NULL), ctk_lps(), dmg_lp(NULL) { }

        void new_turn();
        void play_turn();
        void switch_side(side_number side);
        std::string describe_self() const;
        void on_create();

	virtual config to_config() const;

private:
        clock_t c_0;

        //std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;

#ifdef MCLP_DEBUG
        char cstr[40];
        char file_name[20];
        int file_counter;
#endif
        lprec* temp_lp;
        std::map<const map_location, ctkLP*> ctk_lps;
        damageLP *dmg_lp;

        void buildLPs();
};

/** A test to visualize the output of first LP we solve. */
//class lp_1_ai : public readwrite_context_proxy, public interface {
class lp_1_ai : public ai_composite {
public:
//        lp_1_ai(readwrite_context &context, const config &cfg);
//        lp_1_ai(default_ai_context &context, const config &cfg);
        lp_1_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg) { }
//        lp_1_ai(const lp_1_ai &ai);

        void new_turn();
        void play_turn();
        void switch_side(side_number side);
        std::string describe_self() const;
        //void add stage? iirc ai_composite.play_turn is where the stages are executed so we can just ignore stages now.
        void on_create();

	virtual config to_config() const;
};

/** A test to visualize the output of second LP we solve. */
//class lp_2_ai : public readwrite_context_proxy, public interface {
class lp_2_ai : public ai_composite {
public:
//        lp_2_ai(readwrite_context &context, const config &cfg);
//        lp_2_ai(default_ai_context &context, const config &cfg);
        lp_2_ai(default_ai_context &context, const config& cfg):ai_composite(context, cfg) { }
//        lp_2_ai(const lp_2_ai &ai);

        void new_turn();
        void play_turn();
        void switch_side(side_number side);
        std::string describe_self() const;
        void on_create();

	virtual config to_config() const;
};


} //end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif
