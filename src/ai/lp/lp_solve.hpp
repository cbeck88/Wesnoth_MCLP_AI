#ifndef AI_LP_SOLVE_HPP_INCLUDED
#define AI_LP_SOLVE_HPP_INCLUDED

//Enable this to get full MCLP debugging output
//#define MCLP_DEBUG

#ifdef MCLP_DEBUG
#define LP_SOLVE_LOG_MODE LP_SOLVE_FULL
#else
#define LP_SOLVE_LOG_MODE LP_SOLVE_IMPORTANT
#endif

//#define LP_SET_LOWBO // turn this own to enable set lowbo flags. defaults of >= 0 for all seems fine to me.

//#include <fstream>

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
        unsigned char is_add_rowmode(lprec*);
        unsigned char add_constraint(lprec*,REAL*,int, REAL);
        unsigned char add_constraintex(lprec*,int,REAL*,int*, int, REAL);
        unsigned char set_binary(lprec*,int, unsigned char);
        unsigned char set_obj(lprec*,int,REAL);
        unsigned char set_lowbo(lprec*, int, REAL);
        unsigned char set_col_name(lprec*,int, char*);
        unsigned char del_column(lprec*,int);
        void set_maxim(lprec*);
        void set_minim(lprec*);
        void set_verbose(lprec*, int);
        int solve(lprec*);
        unsigned char get_variables(lprec*, REAL*);
        unsigned char get_ptr_variables(lprec*, REAL**);
        REAL get_objective(lprec*);
        REAL get_lowbo(lprec*, int);
        void delete_lp(lprec*);

        //unsigned char write_LP(lprec*, FILE*);
        unsigned char write_lp(lprec*, char*);

        const char * SOLVE_CODE (int code);
    }
}
//codes for ADD_CONSTRAINT
#define LP_SOLVE_LE 1 //int LE = 1;
#define LP_SOLVE_GE 2 //int GE = 2;
#define LP_SOLVE_EQ 3 //int EQ = 3;

//codes for SET_BINARY
#define LP_SOLVE_TRUE 1  //unsigned char TRUE = 1;
#define LP_SOLVE_FALSE 0 //unsigned char FALSE = 0;

//codes for SET_VERBOSE
#define LP_SOLVE_NEUTRAL 0   //int NEUTRAL = 0;
#define LP_SOLVE_CRITICAL 1  //int CRITICAL = 1;
#define LP_SOLVE_SEVERE 2    //int SEVERE = 2;
#define LP_SOLVE_IMPORTANT 3 //int IMPORTANT = 3;
#define LP_SOLVE_NORMAL 4    //int NORMAL = 4;
#define LP_SOLVE_DETAILED 5  //int DETAILED = 5;
#define LP_SOLVE_FULL 6      //int FULL = 6;

//codes for SOLVE
#define LP_SOLVE_NOMEMORY  -2//int NOMEMORY = -2;
#define LP_SOLVE_OPTIMAL    0//int OPTIMAL = 0;
#define LP_SOLVE_SUBOPTIMAL 1//int SUBOPTIMAL = 1;
#define LP_SOLVE_INFEASIBLE 2//int INFEASIBLE = 2;
#define LP_SOLVE_UNBOUNDED  3//int UNBOUNDED = 3;
#define LP_SOLVE_DEGENERATE 4//int DEGENERATE = 4;
#define LP_SOLVE_NUMFAILURE 5//int NUMFAILURE = 5;

#endif
