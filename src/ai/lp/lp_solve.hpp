#ifndef AI_LP_SOLVE_HPP_INCLUDED
#define AI_LP_SOLVE_HPP_INCLUDED

#include <fstream>

#ifndef REAL 
   #define REAL double
#endif

typedef struct _lprec      lprec;

namespace lp_solve
{
    extern "C"
    {
        lprec* make_lp(int, int);        
        unsigned char set_add_rowmode(lprec*, unsigned char);
        unsigned char add_constraint(lprec*,REAL*,int, REAL);
        unsigned char add_constraintex(lprec*,int,REAL*,int*, int, REAL);
        unsigned char set_binary(lprec*,int, unsigned char);
        unsigned char set_obj(lprec*,int,REAL);
        unsigned char set_lowbo(lprec*, int, REAL);
        unsigned char set_col_name(lprec*,int, char*);
        void set_maxim(lprec*);
        void set_minim(lprec*);
        void set_verbose(lprec*, int);
        int solve(lprec*);
        unsigned char get_variables(lprec*, REAL*);
        REAL get_objective(lprec*);
        void delete_lp(lprec*);

        unsigned char write_LP(lprec*, FILE*);
        unsigned char write_lp(lprec*, char*);

    }
        //codes for ADD_CONSTRAINT
        int LE = 1;
        int GE = 2;
        int EQ = 3;

        //codes for SET_BINARY
        unsigned char TRUE = 1;
        unsigned char FALSE = 0;

        //codes for SET_VERBOSE
        int NEUTRAL = 0;
        int CRITICAL = 1;
        int SEVERE = 2;
        int IMPORTANT = 3;
        int NORMAL = 4;
        int DETAILED = 5;
        int FULL = 6;

        //codes for SOLVE
        int NOMEMORY = -2;
        int OPTIMAL = 0;
        int SUBOPTIMAL = 1;
        int INFEASIBLE = 2;
        int UNBOUNDED = 3;
        int DEGENERATE = 4;
        int NUMFAILURE = 5;

}

#endif
