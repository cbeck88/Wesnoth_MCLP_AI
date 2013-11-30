#include "lp_solve.hpp"

namespace lp_solve
{
    const static char * LP_SOLVE_NOMEMORY_STR = "NOMEMORY";

    const static char * LP_SOLVE_ERR_STRS[] = {"OPTIMAL", "SUBOPTIMAL", "INFEASIBLE", "UNBOUNDED", "DEGENERATE", "NUMFAILURE"};

    const char * SOLVE_CODE (int code) 
    { 
        switch ( code ) {
	case LP_SOLVE_NOMEMORY:   return LP_SOLVE_NOMEMORY_STR;  //int NOMEMORY = -2;
	case LP_SOLVE_OPTIMAL:    //int OPTIMAL = 0;
	case LP_SOLVE_SUBOPTIMAL: //int SUBOPTIMAL = 1;
	case LP_SOLVE_INFEASIBLE: //int INFEASIBLE = 2;
	case LP_SOLVE_UNBOUNDED : //int UNBOUNDED = 3;
	case LP_SOLVE_DEGENERATE: //int DEGENERATE = 4;
	case LP_SOLVE_NUMFAILURE: return LP_SOLVE_ERR_STRS[code];//int NUMFAILURE = 5;
        default: return "BAD CODE";
        }
    }
}
