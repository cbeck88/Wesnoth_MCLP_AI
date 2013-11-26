#include "lp.hpp"

static lg::log_domain log_ai("ai/general");
#define ERR_AI LOG_STREAM(err, log_ai)

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

LP::LP(int n): Ncol(n),lp(NULL)
{
    lp = lp_solve::make_lp(0,n);
    assert(lp != NULL);

    lp_solve::set_add_rowmode(lp, true);
}

FracLP::FracLP(int n): Ncol(n),lp(NULL),not_solved_yet(true)
{   //Ncol is initialized to n, but we need Ncol + 1 for sizing with lp_solve.
    denom_row = (REAL*) malloc( (++n) * sizeof(denom_row));
    lp = lp_solve::make_lp(0,n); //need to hold t variable in last column
    assert(lp != NULL);

    lp_solve::set_add_rowmode(lp, true);

#ifdef MCLP_DEBUG
    lp_solve::set_col_name(lp, n, (char *)"t");
#endif
    //Now add fractional constraint: t >= 0;
    lp_solve::set_lowbo(lp, n, 0);
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
    if (lp_solve::set_lowbo(lp, col, (REAL) 0) != LP_SOLVE_TRUE) 
        return LP_SOLVE_FALSE;

    int shortcol[2] = {col, Ncol+1}; //should be const but lib lp solve doesn't like that.
//  const REAL FracLP::shortcol = {1,-1};
    return lp_solve::add_constraintex(lp, 2, shortrow, shortcol, LP_SOLVE_LE, (REAL) 0); //Constraint is x_i <= t. We cache these inputs in shortrow, shortcol.
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
    int j = 0;
    int *col = (int*) malloc(cnt * sizeof(*col));
    REAL *row = (REAL*) malloc(cnt * sizeof(*row));
    unsigned char ret;

    //for each column in iterator, add a 1 in the row for this constraint
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        ++iter;
    }
    assert(j==cnt);

    ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 1);  //<= 1 constraint in this row, zeros elsewhere
    //assert(ret);
    free(col);
    free(row);

    return(ret);
}

//template<class T> 
unsigned char FracLP::row_LE_1(Map_Itor iter, Map_Itor end, int cnt)
{
    assert(cnt > 0);

    cnt++; // +1, need to include a spot for t variable.
        
    int j = 0;
    int *col = (int*) malloc(cnt * sizeof(*col));
    REAL *row = (REAL*) malloc(cnt * sizeof(*row));
    unsigned char ret;
                
    //for each entry in this range, add a 1 in the row for this constraint, at the column specified at value
    while (iter != end) {
        col[j] = *(iter->second);
        row[j++] = (REAL) 1.0;
        ++iter;
    }
    col[j] = Ncol+1;  //add a -1 for t
    row[j++] = (REAL) -1.0; 
    assert(j == cnt);

    ret = lp_solve::add_constraintex(lp,j,row,col, LP_SOLVE_LE, 0); //The constraint is Ax <= bt
    //assert(ret);
    free(col);
    free(row);

    return ret;
}

unsigned char LP::solve()
{
    if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
        ERR_AI << "LP::solve() failed to set rowmode to false" << std::endl; 
        return LP_SOLVE_NOMEMORY;
    } //the template file suggested to turn this off before solving.

    lp_solve::set_maxim(lp);
    lp_solve::set_verbose(lp, LP_SOLVE_LOG_MODE);

    unsigned char ret = lp_solve::solve(lp);
    if (ret != LP_SOLVE_OPTIMAL) {
        ERR_AI << "LP::solve() solve gave an error code " << ret << std::endl;
    }

    if (lp_solve::get_ptr_variables(lp,vars) != LP_SOLVE_TRUE) {
        ERR_AI << "LP::solve() failed to get variables" << std::endl;
    }
    return ret;
}

unsigned char FracLP::solve()
{
    not_solved_yet = false;

    if (lp_solve::add_constraint(lp, denom_row, LP_SOLVE_EQ, 1) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::solve() failed to add denominator constraint" << std::endl; 
        return LP_SOLVE_NOMEMORY;
    } //this adds d^T y + beta * t = 1
    
    if (lp_solve::set_add_rowmode(lp,LP_SOLVE_FALSE) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::solve() failed to set rowmode to false" << std::endl; 
        return LP_SOLVE_NOMEMORY;
    } //the template file suggested to turn this off before solving.

    lp_solve::set_maxim(lp);
    lp_solve::set_verbose(lp, LP_SOLVE_LOG_MODE);

    unsigned char ret = lp_solve::solve(lp);
    if (ret != LP_SOLVE_OPTIMAL) {
        ERR_AI << "FracLP::solve() solve gave an error code " << ret << std::endl;
    }

    if (lp_solve::get_variables(lp,denom_row) != LP_SOLVE_TRUE) {
        ERR_AI << "FracLP::solve() failed to get variables" << std::endl;
    };
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
    return lp_solve::set_obj(lp,Ncol+1, val);
}

unsigned char FracLP::set_obj_denom(int col, REAL val)
{
    denom_row[col] = val;
    return LP_SOLVE_TRUE;
}

unsigned char FracLP::set_obj_denom_constant(REAL val)
{
    denom_row[Ncol+1]=val;
    return LP_SOLVE_TRUE;
}

REAL LP::get_var(int i)
{
    if (vars == NULL) {ERR_AI << "LP::get_var before solve" << std::endl; return 0;}
    return (*vars)[i];
}

REAL FracLP::get_var(int i)
{
    if (not_solved_yet) {ERR_AI << "LP::get_var before solve" << std::endl; return 0;}
    return denom_row[i];
}

REAL LP::get_obj()
{
    return lp_solve::get_objective(lp);
}

REAL FracLP::get_obj()
{
    return lp_solve::get_objective(lp);
}