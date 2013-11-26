#ifndef INCLUDE_LP_HPP
#define INCLUDE_LP_HPP

#ifndef LP_SOLVE_LOG_MODE 
#define LP_SOLVE_LOG_MODE LP_SOLVE_IMPORTANT
#endif

#include "lp_solve.hpp"
#include "../../map_location.hpp"
#include "../../log.hpp"
#include <stdio.h>

#include <list>
//#include <multimap>
#include <iterator>
#include <algorithm>

typedef std::list<int>::iterator int_ptr;
class map_location;
typedef const map_location T;

//typedef move_map::const_iterator Itor;
typedef std::multimap<T,int_ptr>::iterator Map_Itor;

//sadly, cannot use virtual and templates together ><
/*
class Boolean_Program {
public: 
    void rows_LE_1 (std::multimap<T,int_ptr> *rowset);
    virtual unsigned char row_LE_1(Map_Itor, Map_Itor, int);
//    virtual unsigned char set_lowbo(int, REAL);
//    virtual unsigned char set_upbo(int, REAL);
    virtual unsigned char set_boolean(int);
    virtual unsigned char delete_col(int);
    virtual unsigned char set_col_name(int, char*);
    virtual int end();
};*/


//LP is a wrapper for a linear program with boolean variables and exlclusivity constraints.
class LP /*: public Boolean_Program*/ {
public:
    LP(int);
    ~LP() { if (lp != NULL) {lp_solve::delete_lp(lp);}}
   
    unsigned char row_LE_1(Map_Itor,Map_Itor,int);
    void rows_LE_1 (std::multimap<T,int_ptr> *rowset);
    unsigned char set_boolean(int);
    unsigned char delete_col(int);
    int end() {return Ncol;}
    
    unsigned char set_obj(int, REAL);
    unsigned char solve();

    unsigned char set_col_name(int, char*);   
    REAL get_var(int);
    REAL get_obj();

    unsigned char write_lp(char *);
private:
    int Ncol;
    lprec* lp;
    REAL **vars;
};

// FracLP is a wrapper implementing linear fractional programming in terms of linear programming.
// See wikipedia linear fractional programming.

class FracLP /*: public Boolean_Program*/ {
public:
    FracLP(int n);
    ~FracLP() { if (lp != NULL) {lp_solve::delete_lp(lp); free(denom_row);}}

    unsigned char row_LE_1(Map_Itor,Map_Itor,int);
    void rows_LE_1 (std::multimap<T,int_ptr> *rowset);
    unsigned char set_boolean(int);
    unsigned char delete_col(int);
    int end() {return Ncol;}
    
    unsigned char set_obj_num(int, REAL);
    unsigned char set_obj_denom(int, REAL);
    unsigned char set_obj_num_constant(REAL);
    unsigned char set_obj_denom_constant(REAL);

    unsigned char solve();

    unsigned char set_col_name(int, char*);    
    REAL get_var(int);
    REAL get_obj();

    unsigned char write_lp(char *);
private:
    int Ncol;
    lprec* lp;

    static REAL shortrow[]; //used to implement set_boolean. should be const but lib lp solve doesn't like that.

    REAL* denom_row; //used to hold the denominator constraint, and after solve is called, holds the results. this is a bit hacky.
    bool not_solved_yet; //in this implementation you are not allowed to change denominator entries after calling solve().
};

//
// max c^T * x + \alpha / d^T x + \beta
// Ax \leq b
//
// is equivalent to
// 
// max c^T * y + \alpha t
// Ax \leq bt
// t \geq 0
// d^T y + \beta t = 1

#endif
