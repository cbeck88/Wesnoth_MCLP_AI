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

#include "helper.hpp"
//#include "lp_solve.hpp"
#include "../contexts.hpp"
#include "../interface.hpp"
#include "../composite/ai.hpp"
#include "../../pathfind/pathfind.hpp"

#include <boost/shared_ptr.hpp>

#ifndef REAL 
   #define REAL double
#endif

//Remove this and logs when done debuggins
//#include "../../log.hpp"

//static lg::log_domain log_ai("ai/general");
//this in lp.cpp now 
//#define DBG_AI LOG_STREAM(debug, log_ai)
//#define LOG_AI LOG_STREAM(info, log_ai)
//#define WRN_AI LOG_STREAM(warn, log_ai)
//#define ERR_AI LOG_STREAM(err, log_ai)


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

/** An ai that uses LP heuristics to try to find the most efficiently aggressive move. */

class lp_ai : public ai_composite {
public:
        lp_ai(default_ai_context &context, const config& cfg); //:ai_composite(context, cfg), temp_lp(NULL), ctk_lps(), dmg_lp(NULL) { }

        void new_turn();
        void play_turn();
        void switch_side(side_number side);
        std::string describe_self() const;
        void on_create();

	virtual config to_config() const;

private:
        boost::shared_ptr<ctkLP> temp_lp;
        typedef std::pair< boost::shared_ptr<ctkLP> , ctkLP::iterator  > ctk_pod;
        std::map<const map_location, ctk_pod > ctk_lps;
        boost::shared_ptr<damageLP> dmg_lp;

        std::map<map_location,pathfind::paths> possible_moves;
        move_map srcdst, dstsrc;
	unit_map * units_;

        void buildLPs();
};

/** A test to visualize the output of first LP we solve. */
//class lp_1_ai : public readwrite_context_proxy, public interface {
class lp_1_ai : public ai_composite {
public:
//        lp_1_ai(readwrite_context &context, const config &cfg);
//        lp_1_ai(default_ai_context &context, const config &cfg);
        lp_1_ai(default_ai_context &context, const config& cfg);//:ai_composite(context, cfg) { }
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
        lp_2_ai(default_ai_context &context, const config& cfg);//:ai_composite(context, cfg) { }
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
