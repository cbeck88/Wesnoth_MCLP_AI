#include "lp.hpp"

static lg::log_domain log_ai("ai/general");
#define ERR_AI LOG_STREAM(err, log_ai)
#define DBG_AI LOG_STREAM(debug, log_ai)

REAL FracLP::shortrow[] = {1,-1};

//template <class T>
//void Boolean_Program::

#define rows_LE_1_BODY                                                                                                                 \
rows_LE_1 (std::multimap<T,int_ptr> *rows)                                                                                             \
{                                                                                                                                      \
    std::pair<Map_Itor, Map_Itor> range;                                                                                               \
    for (range = rows->equal_range(rows->begin()->first); range.first != range.second; range = rows->equal_range(range.second->first)) \
    {                                                                                                                                  \
        row_LE_1(range.first, range.second, rows->count(range.first->first));                                                          \
    }                                                                                                                                  \
}                                              
 
void LP::rows_LE_1_BODY
void FracLP::rows_LE_1_BODY

#ifdef MCLP_DEBUG_OVERKILL      
#define SPAM_GET_LOWBO \
    for (int ZZZ = 1 ; ZZZ <= Ncol+1; ZZZ++) { \
        DBG_AI << "lp.get_lowbo(" << ZZZ << ") = " << lp_solve::get_lowbo(lp,ZZZ) << std::endl; }
#else
#define SPAM_GET_LOWBO
#endif         

LP::LP(int n): Ncol(n),lp(NULL),vars(NULL)
{
    lp = lp_solve::make_lp(0,n);
    assert(lp != NULL);

    lp_solve::set_add_rowmode(lp, true);
}
//add_rowmode remarks.
//Default, this is FALSE, meaning that add_column, add_columnex, str_add_column performs best. If the model is build via add_constraint, add_constraintex, str_add_constraint calls, then these routines will be much faster if this routine is called with turnon set on TRUE. This is also called row entry mode. The speed improvement is spectacular, especially for bigger models, so it is advisable to call this routine to set the mode. Normally a model is build either column by column or row by row.
//Note that there are several restrictions with this mode:
//Only use this function after a make_lp call. Not when the model is read from file. Also, if this function is used, first add the objective function via set_obj_fn, set_obj_fnex, str_set_obj_fn and after that add the constraints via add_constraint, add_constraintex, str_add_constraint. Don't call other API functions while in row entry mode. No other data matrix access is allowed while in row entry mode. After adding the contraints, turn row entry mode back off. Once turned of, you cannot switch back to row entry mode. So in short:
//- turn row entry mode on
//- set the objective function
//- create the constraints
//- turn row entry mode off

FracLP::FracLP(int n): Ncol(n),lp(NULL),not_solved_yet(true)
{   //Ncol is initialized to n, but we need Ncol + 1 for sizing with lp_solve.
//    DBG_AI << "FracLP(" << n << ")" << std::endl;
    denom_row = (REAL*) malloc( (n+2) * sizeof(denom_row)); 
//    DBG_AI << "denom_row malloced: " << n << std::endl;
    lp = lp_solve::make_lp(0,n+1); //need to hold t variable in last column
//    DBG_AI << "lp made: " << n << std::endl;
//    DBG_AI << "Ncol = " << Ncol << std::endl;

    assert(lp != NULL);


    DBG_AI << "Just constructed, here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO
    //Now add fractional constraint: t >= 0;
    //Apparently lowbo = 0 is the default... see the API, very strange. Actually I don't believe them.
#ifdef LP_SET_LOWBO
    lp_solve::set_lowbo(lp, Ncol+1, (REAL) 0.0);
#endif


    //lp_solve::set_add_rowmode(lp, LP_SOLVE_TRUE);
#ifdef MCLP_DEBUG
    lp_solve::set_col_name(lp, n+1, (char *)"t");
#endif

    DBG_AI << "set Ncol+1" << (Ncol+1) << ", here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO
}

FracLP::~FracLP() {
#ifdef MCLP_DEBUG
    DBG_AI << "~FracLP();" << std::endl;
#endif
}

unsigned char LP::set_col_name(int col, char* str)
{
    return lp_solve::set_col_name(lp, col, str);
}

unsigned char FracLP::set_col_name(int col, char* str)
{
    return lp_solve::set_col_name(lp, col, str);
}

unsigned char LP::set_boolean(int col)
{
    return lp_solve::set_binary(lp, col, LP_SOLVE_TRUE);
}

unsigned char FracLP::set_boolean(int col)
{
#ifdef MCLP_DEBUG
    DBG_AI << "FracLP::set_boolean. Ncol = " << Ncol << std::endl;
    SPAM_GET_LOWBO
#endif

#ifdef LP_SET_LOWBO
    if (lp_solve::set_lowbo(lp, col, (REAL) 0) != LP_SOLVE_TRUE) 
        return LP_SOLVE_NOMEMORY;
#endif

    REAL shortrow_trial[2] = {1,-1}; //wondering if the static pass is causing memory corruption
    int shortcol[2] = {col, Ncol+1}; //should be const but lib lp solve doesn't like that.
//  const REAL FracLP::shortcol = {1,-1};
    return lp_solve::add_constraintex(lp, 2, shortrow_trial, shortcol, LP_SOLVE_LE, (REAL) 0); //Constraint is x_i <= t. We cache these inputs in shortrow, shortcol.
} //todo: merge the error codes somehow...???

unsigned char LP::delete_col(int col)
{
    return lp_solve::del_column(lp, col);
}

unsigned char FracLP::delete_col(int col)
{
    return lp_solve::del_column(lp, col);
}

//template<class T>
unsigned char LP::row_LE_1(Map_Itor iter, Map_Itor end, int cnt)
{
    assert(cnt > 0);
    int j = 1;
    int *col = (int*) malloc((cnt+1) * sizeof(*col));
    REAL *row = (REAL*) malloc((cnt+1) * sizeof(*row));
    unsigned char ret;

    //for each column in iterator, add a 1 in the row for this constraint
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        ++iter;
    }
    assert(j==(cnt+1)); // j = 1 + one for every input element, should be cnt + 1

    ret = lp_solve::add_constraintex(lp,j+1,row,col, LP_SOLVE_LE, 1);  //<= 1 constraint in this row, zeros elsewhere
    //assert(ret);
    free(col);
    free(row);

    return(ret);
}

//template<class T> 
unsigned char FracLP::row_LE_1(Map_Itor iter, Map_Itor end, int cnt)
{
    DBG_AI << "Row add start: Ncol = " << Ncol << std::endl;

    assert(cnt > 0);

    //cnt++; cnt++// +2, need to include a spot for t variable, and be 1 based
        
    #define BASE_ARRAYS_FROM 0
    int j = BASE_ARRAYS_FROM;//1;
    int *col = (int*) malloc((cnt+2) * sizeof(*col));
    REAL *row = (REAL*) malloc((cnt+2) * sizeof(*row));
    unsigned char ret;
                
    //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        DBG_AI << "col[" << j-1 << "] = " << col[j-1] << "\trow[" << j-1 << "] = " << row[j-1] << std::endl;
        ++iter;
    }
    col[j] = Ncol+1;  //add a -1 for t
    row[j] = (REAL) -1.0; 
    DBG_AI << "col[" << j << "] = " << col[j] << "\trow[" << j << "] = " << row[j] << std::endl;
    assert(j == (cnt+BASE_ARRAYS_FROM)); //j = BASE_ARRAYS_FROM + 1 for every input element, + 1 again, should be cnt + BASE_ARRAYS_FROM

    DBG_AI << "add(lp," << j+1 << ", row, col <= 0);" << std::endl;

    ret = lp_solve::add_constraintex(lp,j+1,row,col, LP_SOLVE_LE, 0); //The constraint is Ax <= bt
    //assert(ret);
    free(col);
    free(row);

//    DBG_AI << "Row add end: Ncol = " << Ncol << std::endl;
    return ret;
}

unsigned char LP::finishRows()
{
#ifdef MCLP_DEBUG
    if (lp_solve::is_add_rowmode(lp) == LP_SOLVE_TRUE)
    {
        DBG_AI << "LP::finishRows already had rowmode false... returning." << std::endl;
        return LP_SOLVE_TRUE;
    }
#endif
    if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
        ERR_AI << "LP::finishRows failed to set rowmode to false" << std::endl; 
        return LP_SOLVE_FALSE;
    } //the template file suggested to turn this off before solving.
    return LP_SOLVE_TRUE;
}

unsigned char LP::solve()
{
    lp_solve::set_maxim(lp);
    lp_solve::set_verbose(lp, LP_SOLVE_LOG_MODE);

    if (lp_solve::is_add_rowmode(lp) == LP_SOLVE_TRUE) {
        DBG_AI << "LP::solve Hmm add row mode is on for some reason... turning off." << std::endl;
        if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
            ERR_AI << "LP::solve failed to set rowmode to false" << std::endl; 
            return LP_SOLVE_FALSE;
        }
    } //the template file suggested to turn this off before solving.

    unsigned char ret = lp_solve::solve(lp);
    if (ret != LP_SOLVE_OPTIMAL) {
        ERR_AI << "LP::solve() solve gave an error code " << ret << std::endl;
    }

    if (lp_solve::get_ptr_variables(lp,&vars) != LP_SOLVE_TRUE) {
        ERR_AI << "LP::solve() failed to get variables" << std::endl;
    }
    return ret;
}

unsigned char FracLP::finishRows()
{
    DBG_AI << "LP::finishRows(), here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO

    if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::finishRows failed to set rowmode to false" << std::endl; 
        return LP_SOLVE_FALSE;
    } //the template file suggested to turn this off before solving.
    return LP_SOLVE_TRUE;
}

unsigned char FracLP::solve()
{
    DBG_AI << "FracLP::solve()'ing." << std::endl;
//    DBG_AI << "Ncol = " << Ncol << std::endl;
//    DBG_AI << "here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO

    not_solved_yet = false;

#ifdef LP_SET_LOWBO
    lp_solve::set_lowbo(lp, Ncol+1, (REAL) 0.0);
#endif


    DBG_AI << "About to commit denom_row:" << std::endl;
    for(int i =1; i <= Ncol+1; i++)
    { 
        DBG_AI << "denom_row["<< i << "]=" << denom_row[i] << std::endl;
    }

    if (lp_solve::add_constraintex(lp, Ncol+1,denom_row,NULL, LP_SOLVE_EQ, 1) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::solve() failed to add denominator constraint" << std::endl; 
        return LP_SOLVE_NOMEMORY;
    } //this adds d^T y + beta * t = 1
    
    if (lp_solve::is_add_rowmode(lp) == LP_SOLVE_TRUE) {
        DBG_AI << "FracLP::solve Hmm add row mode is on for some reason... turning off." << std::endl;
        if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
            ERR_AI << "FracLP::solve failed to set rowmode to false" << std::endl; 
            return LP_SOLVE_FALSE;
        }
    } //the template file suggested to turn this off before solving.

#ifdef LP_SET_LOWBO
    lp_solve::set_lowbo(lp, Ncol+1, (REAL) 0.0);
#endif

    lp_solve::set_maxim(lp);
    lp_solve::set_verbose(lp, LP_SOLVE_LOG_MODE);

    unsigned char ret = lp_solve::solve(lp);
    if (ret != LP_SOLVE_OPTIMAL) {
        ERR_AI << "FracLP::solve() solve gave an error code " << ret << std::endl;
    }

    if (lp_solve::get_variables(lp,denom_row) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::solve() failed to get variables" << std::endl;
    };
    DBG_AI << "Done: Ncol = " << Ncol << std::endl;
    DBG_AI << "LP::solve() done, here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO

    return ret;
}

unsigned char LP::set_obj(int col, REAL val)
{
    return lp_solve::set_obj(lp,col, val);
}

unsigned char FracLP::set_obj_num(int col, REAL val)
{
    return lp_solve::set_obj(lp,col,val);
}

unsigned char FracLP::set_obj_num_constant(REAL val)
{
//    DBG_AI << "FracLP::Set num constant " << val << std::endl;
//    DBG_AI << "Ncol = " << Ncol << std::endl;
    return lp_solve::set_obj(lp,Ncol+1, val);
}

unsigned char FracLP::set_obj_denom(int col, REAL val)
{
//    DBG_AI << "denom_row[" << col << "]=" << val << std::endl;
//    DBG_AI << "denom_row[end]=" << denom_row[Ncol] << std::endl;
    denom_row[col] = val;
    return LP_SOLVE_TRUE;
}

unsigned char FracLP::set_obj_denom_constant(REAL val)
{
//    DBG_AI << "denom_row[" << Ncol+1 << "]=" << val << std::endl;
    denom_row[Ncol+1]=val;
    return LP_SOLVE_TRUE;
}

REAL LP::get_var(int i)
{
    if (vars == NULL) {ERR_AI << "LP::get_var before solve" << std::endl; return 0;}
    return vars[i-1];
}

REAL FracLP::get_var(int i)
{
    if (not_solved_yet) {ERR_AI << "LP::get_var before solve" << std::endl; return 0;}
    return denom_row[i-1]/denom_row[Ncol];
}

bool FracLP::var_gtr(int i, REAL eps)
{
    if (not_solved_yet) {ERR_AI << "LP::get_var before solve" << std::endl; return 0;}
    return (denom_row[i-1] > (eps * denom_row[Ncol]));
}


REAL LP::get_obj()
{
    return lp_solve::get_objective(lp);
}

REAL FracLP::get_obj()
{
    return lp_solve::get_objective(lp);
}

unsigned char LP::write_lp(char * file)
{
    return lp_solve::write_lp(lp,file);
}

unsigned char FracLP::write_lp(char * file)
{
    DBG_AI << "FracLP::write_lp(), here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO

    return lp_solve::write_lp(lp,file);
}
