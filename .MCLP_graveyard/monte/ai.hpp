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

#ifndef AI_MONTE_AI_HPP_INCLUDED
#define AI_MONTE_AI_HPP_INCLUDED

#include "../interface.hpp"
#include "../composite/stage.hpp"

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif


namespace pathfind {

struct plain_route;

} // of namespace pathfind


namespace ai {

/** An ai that tries to plan its turns using monte carlo simulations. */
class monte_ai : public readwrite_context_proxy, public interface {
public:
	virtual config to_config() const;
};


} //end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif
