#include "lp.hpp"
#include "lp_solve.hpp"

//static lg::log_domain log_ai("ai/general");
//#define ERR_AI LOG_STREAM(err, log_ai)
//#define DBG_AI LOG_STREAM(debug, log_ai)

REAL FracLP::shortrow[] = {1,-1};

#ifdef MCLP_DEBUG_OVERKILL      
#define SPAM_GET_LOWBO \
    for (int ZZZ = 1 ; ZZZ <= Ncol+1; ZZZ++) { \
        DBG_AI << "lp.get_lowbo(" << ZZZ << ") = " << lp_solve::get_lowbo(lp,ZZZ) << std::endl; }
#else
#define SPAM_GET_LOWBO
#endif         

LP::LP(int n): Ncol(n),lp(NULL),vars(NULL)
{
    DBG_AI << "LP::LP();" << std::endl;
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

//Turn this back on after fixed memory problems.
//FracLP::~FracLP() { if (lp != NULL) {lp_solve::delete_lp(lp);} if (denom_row != NULL) {free(denom_row);}}

//FracLP::~FracLP() {
//#ifdef MCLP_DEBUG
//    DBG_AI << "~FracLP();" << std::endl;
//#endif
//}

LP::LP(LP& copy_from_me)
{
    DBG_AI << "LP_copy: incoming Ncol = " << copy_from_me.Ncol << std::endl;
    Ncol = copy_from_me.Ncol;
    vars = NULL;
    lp = lp_solve::copy_lp(copy_from_me.lp);
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

    REAL shortrow_trial[2] = {1,-1}; //wondering if the static pass is causing memory corruption. (this is outdated, i don't think it was...)
    int shortcol[2] = {col, Ncol+1}; //should be const but lib lp solve doesn't like that.
//  const REAL FracLP::shortcol = {1,-1};
    return lp_solve::add_constraintex(lp, 2, shortrow_trial, shortcol, LP_SOLVE_LE, (REAL) 0); //Constraint is x_i <= t. We cache these inputs in shortrow, shortcol.
} //todo: merge the error codes somehow...???

unsigned char LP::delete_col(int col)
{
    assert(col > 0);
    assert(col <= Ncol);
    Ncol--;
    return lp_solve::del_column(lp, col);
}

unsigned char FracLP::delete_col(int col)
{
    assert(col > 0);
    assert(col <= Ncol);
    Ncol--;
    return lp_solve::del_column(lp, col);
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

//Going to move this to get_var
//    if (lp_solve::get_ptr_variables(lp,&vars) != LP_SOLVE_TRUE) {
//        ERR_AI << "LP::solve() failed to get variables" << std::endl;
//    }
    return ret;
}

unsigned char FracLP::finishRows()
{
#ifdef MCLP_DEBUG
    DBG_AI << "LP::finishRows(), here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO
#endif

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
//    SPAM_GET_LOWBO
//    DBG_AI << "not_solved_yet = " << not_solved_yet << std::endl;

    not_solved_yet = false;

#ifdef LP_SET_LOWBO
    lp_solve::set_lowbo(lp, Ncol+1, (REAL) 0.0);
#endif

#ifdef MCLP_DEBUG
    DBG_AI << "About to commit denom_row:" << std::endl;
    for(int i =1; i <= Ncol+1; i++)
    { 
        DBG_AI << "denom_row["<< i << "]=" << denom_row[i] << std::endl;
    }
#endif

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
//    DBG_AI << "Done: Ncol = " << Ncol << std::endl;
//    DBG_AI << "LP::solve() done, here's GETLOWBO:" << std::endl;
//    SPAM_GET_LOWBO
//    DBG_AI << "not_solved_yet = " << not_solved_yet << std::endl;

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
    if (lp_solve::get_ptr_variables(lp,&vars) != LP_SOLVE_TRUE) {
        ERR_AI << "LP::get_var() failed to get_ptr_variables" << std::endl;
    }
    return vars[i-1];
}

REAL FracLP::get_var(int i)
{
    DBG_AI << "FracLP::get_var() -- denom_row[Ncol] = " << denom_row[Ncol] << " -- not_solved_yet = " << not_solved_yet << std::endl;
    if (not_solved_yet == true) {ERR_AI << "FracLP::get_var before solve" << std::endl; return 0;}
    return denom_row[i-1]/denom_row[Ncol];
}

bool FracLP::var_gtr(int i, REAL eps)
{
    if (not_solved_yet == true) {ERR_AI << "FracLP::var_gtr before solve" << std::endl; return 0;}
    return (denom_row[i-1] > (eps * denom_row[Ncol]));
}

REAL LP::get_obj()
{
    return lp_solve::get_objective(lp);
}

REAL FracLP::get_obj()
{
    DBG_AI << "FracLP::get_obj: not_solved_yet = " << not_solved_yet << std::endl;    
    return lp_solve::get_objective(lp);
}

unsigned char LP::write_lp(char * file)
{
    return lp_solve::write_lp(lp,file);
}

unsigned char FracLP::write_lp(char * file)
{
#ifdef MCLP_DEBUG
    DBG_AI << "FracLP::write_lp(), here's GETLOWBO:" << std::endl;
    SPAM_GET_LOWBO
#endif 

    return lp_solve::write_lp(lp,file);
}
