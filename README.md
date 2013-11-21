Wesnoth_MCLP_AI
===============

An AI project for wesnoth. 

What's the deal?
----------------

The idea is that the AI should evaluate the quality of a hypothetical position by running monte carlo simulations. 
During these monte carlo simulations, all parties will be controlled by an efficient and aggressive AI, which seeks
to kill as much gold as possible in its turn. To do this, it solves a series of linear programs related to the game
state, hopefully very quickly. The linear programs seek to approximately determine

1) What is the enemy unit that is easiest for me to kill?

2) What is the opportunity cost of attacking with a particular unit?

The LP_ai heuristic is to *attack the unit which can be most easily killed using all adjacent slots, using the 
unit from the optimal solution which is otherwise the least useful first*. After we see if we got our hits, we reevaluate.

The MCLP_ai is not written yet, but this is the one which is planned to use the results of MC simulations to make decisions.

I may decide to write lua hooks to the Monte Carlo procedures, so that this idea can be reused in the future more easily.

LP's are solved using the lp_solve library. I installed from a linux mint package *liblpsolve55-dev*, following instructions <a href="http://web.mit.edu/lpsolve/doc/Build.htm#Implicit linking with the lpsolve static library ">here</a>.

*If you are having trouble linking it:* For me the package automatically put a library file liblpsolve55.a in /usr/lib/, and according to scons output, g++ is told to look for libraries there, so I didn't have to do anything. I don't know anything about SCONS though so if it doesn't work out of the box good luck :)


Building
--------

Intended to be installed to /src/ai/, the only core file being modified is /ai/registry.cpp. 

In /wesnoth-old/src/SConscript: You must add 

    ai/lp/ai.cpp 
    
to the list of wesnoth sources, with the other ai.cpp files, to get scons to build it.

In /wesnoth-old/SConstruct: You must also change the single line 341:

        conf.CheckOgg() or Warning("Client prerequesites are not met. wesnoth, cutter and exploder cannot be built.")

to the two lines:

        conf.CheckOgg() and \
        conf.CheckLib("liblpsolve55") or Warning("Client prerequisites are not met. wesnoth, cutter and exploder cannot be built.")

to ensure that you include the lp_solve lib.
