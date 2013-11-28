#ifndef INCLUDE_LP_HPP
#define INCLUDE_LP_HPP

#include "lp_solve.hpp"
#include "../../log.hpp"
#include "assert.h"
#include <cstdlib>

#include <map>
#include <list>
#include <iterator>

#ifndef REAL 
   #define REAL double
#endif

static lg::log_domain log_ai("ai/general");
//this in lp.cpp now 
#define DBG_AI LOG_STREAM(debug, log_ai)
#define LOG_AI LOG_STREAM(info, log_ai)
#define WRN_AI LOG_STREAM(warn, log_ai)
#define ERR_AI LOG_STREAM(err, log_ai)

//#define DBG_AI_LP_HPP DBG_AI

//Changed this at some point when I was having problems... 0 seems to be the correct value, 1 leads to segfaults.
#define BASE_ARRAYS_FROM 0


typedef std::list<int>::iterator int_ptr;

//This produces compiler bugs so we need to use a workaround
//template<class T> typedef std::multimap< T,int_ptr >::iterator Itor<T>
template<class T>struct Itor {
    typedef typename std::multimap<T,int_ptr>::iterator type;
};


//class map_location;
//typedef const map_location T;

//typedef move_map::const_iterator Itor;
//typedef std::multimap<T,int_ptr>::iterator Map_Itor;

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
//You can pass it a multimap<T, int_ptr> and it will write linear constraints saying at most
//one variable pointed to by a ptr associated to any given key can be set to 1.

//FracLP wraps a fractional linear program. See wikipedia linear fractional programming.
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

class LP /*: public Boolean_Program*/ {
public:
    LP(int);
    LP(LP & );
    //~LP() { if (lp != NULL) {lp_solve::delete_lp(lp);}}
    //turn this back on after fixed memory problems   

    unsigned char finishRows();
    template <class T> unsigned char row_LE_1 ( typename Itor<T>::type , typename Itor<T>::type , int);
    template <class T> void rows_LE_1 ( typename std::multimap< T , int_ptr > *rowset);
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
    REAL *vars;
};

class FracLP /*: public Boolean_Program*/ {
public:
    FracLP(int n);
    //Turn this back on after fixed memory problems.
    //~FracLP(); // { if (lp != NULL) {lp_solve::delete_lp(lp);} if (denom_row != NULL) {free(denom_row);}}
    //~FracLP();

    unsigned char finishRows();
    template <class T> unsigned char row_LE_1 ( typename Itor<T>::type, typename Itor<T>::type,int);
    template <class T> void rows_LE_1 ( typename std::multimap< T ,int_ptr> *rowset);
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
    bool var_gtr(int, REAL);
    REAL get_obj();

    unsigned char write_lp(char *);
private:
    int Ncol;
    lprec* lp;

    static REAL shortrow[]; //used to implement set_boolean. should be const but lib lp solve doesn't like that.

    REAL* denom_row; //used to hold the denominator constraint, and after solve is called, holds the results. this is a bit hacky.
    bool not_solved_yet; //in this implementation you are not allowed to change denominator entries after calling solve().
};

//Template Voodoo below

template<class T> unsigned char LP::row_LE_1 (typename Itor<T>::type iter, typename Itor<T>::type end, int cnt)
{
    //DBG_AI_LP_HPP<< "Row add start: Ncol = " << Ncol << std::endl;

    assert(cnt > 0);
    int j = BASE_ARRAYS_FROM;
    int *col = (int*) malloc((cnt+BASE_ARRAYS_FROM) * sizeof(*col));
    REAL *row = (REAL*) malloc((cnt+BASE_ARRAYS_FROM) * sizeof(*row));
    unsigned char ret;

    //for each column in iterator, add a 1 in the row for this constraint
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        //DBG_AI_LP_HPP<< "col[" << j-1 << "] = " << col[j-1] << "\trow[" << j-1 << "] = " << row[j-1] << std::endl;
        ++iter;
    }
    assert(j==(cnt+BASE_ARRAYS_FROM)); // j = BASE_ARRAYS_FROM + one for every input element, should be cnt + BASE_ARRAYS_FROM

    //DBG_AI_LP_HPP<< "add(lp," << j << ", row, col <= 1);" << std::endl;

    ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 1);  //<= 1 constraint in this row, zeros elsewhere
    //assert(ret);

    //DBG_AI_LP_HPP<< "row_LE_1 free-ing memory" << std::endl;

    free(col);
    free(row);

    //DBG_AI_LP_HPP<< "Row add end" << std::endl;


    return(ret);
}

template<class T> unsigned char FracLP::row_LE_1 ( typename Itor<T>::type iter, typename Itor<T>::type end, int cnt)
{
    //DBG_AI_LP_HPP<< "Row add start: Ncol = " << Ncol << std::endl;

    assert(cnt > 0);

    //cnt++; cnt++// +2, need to include a spot for t variable, and be 1 based
        
    int j = BASE_ARRAYS_FROM;//1;
    int *col = (int*) malloc((cnt+1+BASE_ARRAYS_FROM) * sizeof(*col));
    REAL *row = (REAL*) malloc((cnt+2) * sizeof(*row));
    unsigned char ret;
                
    //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        //DBG_AI_LP_HPP<< "col[" << j-1 << "] = " << col[j-1] << "\trow[" << j-1 << "] = " << row[j-1] << std::endl;
        ++iter;
    }
    col[j] = Ncol+1;  //add a -1 for t
    row[j++] = (REAL) -1.0; 
    //DBG_AI_LP_HPP<< "col[" << j << "] = " << col[j] << "\trow[" << j << "] = " << row[j] << std::endl;
    assert(j == (cnt+1+BASE_ARRAYS_FROM)); //j = BASE_ARRAYS_FROM + 1 for every input element, + 1 again for t, should be cnt + 1 + BASE_ARRAYS_FROM

    //DBG_AI_LP_HPP<< "add(lp," << j << ", row, col <= 0);" << std::endl;

    ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 0); //The constraint is Ax <= bt
    //assert(ret);
    free(col);
    free(row);

//    DBG_AI_LP_HPP<< "Row add end: Ncol = " << Ncol << std::endl;
    return ret;
}

//This macro, and Itor<T>::type defn, are a hack to get around the fact that templates cannot be used with virtual keyword.

#define rows_LE_1_BODY                                                                                                                 \
rows_LE_1 (typename std::multimap< T , int_ptr > *rows)                                                                                \
{                                                                                                                                      \
    std::pair< typename Itor<T>::type , typename Itor<T>::type > range;                                                                \
    for (range = rows->equal_range(rows->begin()->first); range.first != range.second; range = rows->equal_range(range.second->first)) \
    {                                                                                                                                  \
        row_LE_1 <T> (range.first, range.second, rows->count(range.first->first));                                                     \
    }                                                                                                                                  \
}                                              
 
template <class T> void LP::rows_LE_1_BODY
template <class T> void FracLP::rows_LE_1_BODY

#endif


