Wesnoth_MCLP_AI
===============

An AI project for wesnoth. 

The idea is that the AI should evaluate the quality of a hypothetical position by running monte carlo simulations. 
During these monte carlo simulations, all parties will be controlled by an efficient and aggressive AI, which seeks
to kill as much gold as possible in its turn. To do this, it solves a series of linear programs related to the game
state, hopefully very quickly. The linear programs seek to approximately determine

1. What is the enemy unit that is easiest for me to kill?

2. What is the opportunity cost of attacking with a particular unit?

The LP_ai heuristic is to *attack the unit which can be most easily killed using all adjacent slots, using the 
unit from the optimal solution which is otherwise the least useful first*. After we see if we got our hits, 
reevaluate the same way.

The MCLP_ai is not written yet, but this is the one which is planned to use the results of MC simulations to make 
decisions.

Current AIs
-----------

**lp_1_ai**: This solves an LP for question (2) above. It assigning all units to slots to attack all enemies
in the way which maximizes expected damage. The ai executes these moves to display the optimum solution.
By removing a unit and a slot and recomputing the OPT, you can estimate the opportunity cost of an attack.
(might be a good idea to make a gold-adjusted version of this as well)

**lp_2_ai**: This solves an LP for question (1) above. For each enemy unit, list all possible ways it can be 
attacked and from which slot, then find attack solution which maximizes ctk. We approximate ctk with a fractional LP. 
The ai solves this for every enemy unit, picks the best one and displays the optimal solution by moving 
these units to attack.

Both of these are just tools to visualize the results of these LPs for further testing. 
*These ais will not recruit or grab villages.* 
You should load them from a savegame file which is already set up for an attack.

Future AIs
----------

**lp_ai**: Will be a helper ai using the lps tested in lp_ai_1, lp_ai_2, which seeks to execute turn as aggressively 
as possible.

**mclp_ai**: Will use lp_ai in monte carlo simulations.

Other
-----

There are some test saves and utilities in /testsaves. Use the script aitest to inject an ai into one of the savegames
found there, and load it into wesnoth-1.11. Or add your own replays/ais for testing.

I may decide to write lua hooks to the Monte Carlo procedures, so that this idea can be reused in the future more easily.

LP's are solved using the <a href="http://lpsolve.sourceforge.net/5.0/">lp_solve library</a> 
(<a href="http://lpsolve.sourceforge.net/5.0/lp_solveAPIreference.htm">API</a>, 
<a href="http://lpsolve.sourceforge.net/5.5/formulate.htm#C/C++">example</a>). 
I installed from a linux mint package *liblpsolve55-dev*, 
following instructions <a href="http://web.mit.edu/lpsolve/doc/Build.htm#Implicit linking with the lpsolve static library ">here</a>.

You must have this library to build.

*If you are having trouble linking it:* For me the package automatically put a library file liblpsolve55.a in /usr/lib/, and according to scons output, g++ is told to look for libraries there, 
so I didn't have to do much. I don't know anything about scons though so if it doesn't work out of the box good luck :)
You also need libcolamd2.7.1 package at least. For me this was automatically there. You need glibc (= library ld?) 
version 2.2.5 (if I remember the page correctly). This version is very old so almost surely whatever you have is fine.

Building
--------

I assume that you already have the ability to compile wesnoth from source. If not you can visit our fine friends over 
<a href="https://github.com/wesnoth/wesnoth-old">here</a>.

MCLP AI is essentially a branch of wesnoth-old. I just give it its own github repository like this to keep it at a 
manageable size.

To build MCLP AI, 

1. Make a folder to hold both projects, e.g. wesnoth-src. Clone wesnoth-old and Wesnoth\_MCLP\_AI into this folder.

   (If MCLP_AI base folder is not a sibling of wesnoth-old, the only bad thing is that the testsaves suite won't work.)

2. Now the contents of MCLP AI are intended to be installed (copied) to the base folder wesnoth-old. 

   You might want to "git checkout -b MCLP_AI" in wesnoth-old beforehand.

3. Then just run scons in /wesnoth-old/ as usual.



The core files which are modified are /src/ai/registry.cpp, /src/SConscript, and /SConstruct.

In _/wesnoth-old/src/ai/registry.cpp_: Add my ai factories so that they can be used in game.

    diff --git a/src/ai/registry.cpp b/src/ai/registry.cpp
    index cc57dcf..0435fc4 100644
    --- a/src/ai/registry.cpp
    +++ b/src/ai/registry.cpp
    @@ -48,7 +48,8 @@ static register_ai_factory<ai_composite> ai_factory_default("");
     static register_ai_factory<ai_composite> ai_default_ai_factory("default_ai");
     static register_ai_factory<idle_ai> ai_idle_ai_factory("idle_ai");
     static register_ai_factory<ai_composite> ai_composite_ai_factory("composite_ai");
    +static register_ai_factory<lp_1_ai> ai_lp_1_ai_factory("lp_1_ai");
    +static register_ai_factory<lp_2_ai> ai_lp_2_ai_factory("lp_2_ai");
     
     // =======================================================================
     // Engines

In _/wesnoth-old/src/SConscript_: Add my source .cpp files

    diff --git a/src/SConscript b/src/SConscript
    index fbbe281..e6bc25d 100644
    --- a/src/SConscript
    +++ b/src/SConscript
    @@ -197,6 +197,7 @@ wesnoth_sources = Split("""
         ai/game_info.cpp
         ai/gamestate_observer.cpp
         ai/interface.cpp
    +    ai/lp/ai.cpp
         ai/lua/core.cpp
         ai/lua/lua_object.cpp
         ai/lua/unit_advancements_aspect.cpp
         
         
to the list of wesnoth sources, with the other ai.cpp files, to get scons to build it.

In _/wesnoth-old/SConstruct_: Add my required libraries so that they are checked and statically linked.

    diff --git a/SConstruct b/SConstruct
    index 5d4749c..722a158 100755
    --- a/SConstruct
    +++ b/SConstruct
    @@ -338,7 +338,11 @@ if env["prereqs"]:
             conf.CheckSDL("SDL_mixer", require_version = '1.2.0') and \
             conf.CheckLib("vorbisfile") and \
             conf.CheckSDL("SDL_image", require_version = '1.2.0') and \
    +        conf.CheckLib("liblpsolve55") and \
    +        conf.CheckLib("colamd") and \
    +        conf.CheckLib("dl") and \
             conf.CheckOgg() or Warning("Client prerequisites are not met. wesnoth, cutter and exploder cannot be built.")


I am not going to fuss with required versions in the SConstruct script,
because scons is something of a mystery to me and I don't want to mess around with it
