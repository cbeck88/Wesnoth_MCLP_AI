#include "ai.hpp"
#include "engine.hpp"
#include "rca.hpp"
#include "../../log.hpp"

namespace ai {

MCmodule::MCmodule( game_state new_gs, bool fullturn, int numcycles )
{
    gs = new_gs;
    do_full_turn = fullturn;
    num_cycles_total = numcycles;
    num_cycles_done = 0;
    running_mean = 0;
    running_mean_square = 0;
}

MCmodule::~MCmodule() {};

bool MCmodule::isFinished() {return (num_cycles_total >= num_cycles_done);}
int MCmodule::getNumCyclesTotal() {return num_cycles_total;}
int MCmodule::getNumCyclesDone() {return num_cycles_done;}
bool MCmodule::getFullTurn() {return do_full_turn;}
double MCmodule::getMean() {return running_mean;}
double MCmodule::getVariance() {return running_mean_square - (running_mean*running_mean);}

void MCmodule::execute() 
{
    while (!isFinished())
    {
         execute_one();
    }
}

void MCmodule::execute_one()
{
    // copy state
    
    // seed rng
}

//============================================================================

} // of namespace ai

