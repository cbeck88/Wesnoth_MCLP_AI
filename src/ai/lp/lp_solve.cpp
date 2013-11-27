#include "lp_solve.hpp"

namespace lp_solve
{
    const char * SOLVE_CODE (int code) 
    { 
        switch ( code ) {
	case LP_SOLVE_NOMEMORY:   return "NOMEMORY";  //int NOMEMORY = -2;
	case LP_SOLVE_OPTIMAL:    return "OPTIMAL";   //int OPTIMAL = 0;
	case LP_SOLVE_SUBOPTIMAL: return "SUBOPTIMAL";//int SUBOPTIMAL = 1;
	case LP_SOLVE_INFEASIBLE: return "INFEASIBLE";//int INFEASIBLE = 2;
	case LP_SOLVE_UNBOUNDED : return "UNBOUNDED"; //int UNBOUNDED = 3;
	case LP_SOLVE_DEGENERATE: return "DEGENERATE";//int DEGENERATE = 4;
	case LP_SOLVE_NUMFAILURE: return "NUMFAILURE";//int NUMFAILURE = 5;
        default: return "BAD CODE";
        }
    }
}
