Wesnoth_MCLP_AI
===============

An AI project for wesnoth. Intended to be installed to /src/ai/, the only core file being modified is registry.cpp. 

The idea is that the AI should evaluate the quality of a hypothetical position by running monte carlo simulations. 
During these monte carlo simulations, all parties will be controlled by an efficient and aggressive AI, which seeks
to kill as much gold as possible in its turn. To do this, it solves a series of linear programs related to the game
state, hopefully very quickly. The linear programs seek to approximately determine

1) What is the enemy unit that is easiest for me to kill?

2) What is the opportunity cost of attacking with a particular unit?

The LP_ai heuristic is to *attack the unit which can be most easily killed using all adjacent slots, using the 
unit from the optimal solution which is otherwise the least useful first*. After we see if we got our hits, we reevaluate.

The MCLP_ai will use montecarlo simulations driven by LP_ai to estimate whether it will win a dog fight.

LP's are solved using the lp_solve library. I installed from a linux mint package *liblpsolve55-dev*, following instructions <a href="http://web.mit.edu/lpsolve/doc/Build.htm#Implicit linking with the lpsolve static library ">here</a>.

*If you are having trouble linking it:* For me the package automatically put a library file liblpsolve55.a in /usr/lib/, and according to scons output, g++ is told to look for libraries there, so I didn't have to do anything. I don't know anything about SCONS though so if it doesn't work out of the box good luck :)

