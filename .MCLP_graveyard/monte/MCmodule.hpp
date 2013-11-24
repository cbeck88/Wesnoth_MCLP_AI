/**
 * @file
 * candidate action framework
 */

#ifndef AI_MONTE_AI_MCMODULE_HPP_INCLUDED
#define AI_MONTE_AI_MCMODULE_HPP_INCLUDED

//#include "component.hpp"
//#include "contexts.hpp"

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

//============================================================================
namespace ai {

class MCmodule {

public:
	MCmodule ( game_state new_gs, bool fullturn, int num_cycles );
        virtual ~MCmodule();

        bool isFinished() const;
        int getCyclesTotal() const;
        int getCyclesDone() const;
        bool getFullTurn() const;
        double getMean() const;
        double getVariance() const;

        void execute();
        void execute_one();

private:
        game_state gs;
        int num_cycles_total;
        int num_cycles_done;
        double running_mean;
        double running_variance;
        boolean do_full_turn;

};

} //end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
