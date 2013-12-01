/**
 * @file
 * Artificial intelligence - The computer commands the enemy.
 */


#include "helper.hpp"
#include "lp.hpp"

//#include "ai.hpp"
//#include "lp_solve.hpp"

#include "../game_info.hpp"
#include "../manager.hpp"

#include "../../attack_prediction.hpp"
#include "../../actions/attack.hpp"
#include "../../dialogs.hpp"
#include "../../game_events/pump.hpp"
#include "../../gamestatus.hpp"
#include "../../log.hpp"
#include "../../map_location.hpp"
#include "../../resources.hpp"
#include "../../terrain_filter.hpp"
#include "../../unit_display.hpp"
#include "../../wml_exception.hpp"
//#include "../../pathfind/pathfind.hpp"

#include <stdio.h>
#include <boost/foreach.hpp>

#include <queue>
#include <iterator>
#include <algorithm>

//static lg::log_domain log_ai("ai/general");
//this in lp.hpp now 
//#define DBG_AI LOG_STREAM(debug, log_ai)
//#define LOG_AI LOG_STREAM(info, log_ai)
//#define WRN_AI LOG_STREAM(warn, log_ai)
//#define ERR_AI LOG_STREAM(err, log_ai)

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

namespace ai {

typedef move_map::const_iterator Itor;
typedef std::multimap<map_location,int>::iterator locItor;

//*************************************************************
// Implementation of LP wrappers
//*************************************************************

damageLP::damageLP():lp(), slotMap(), unitMap(), defenderMap(), Ncol(0),cols() {}
ctkLP::ctkLP(map_location ml): defender(ml), lp(), slotMap(), unitMap(),Ncol(0),cols(),made(false), holdingnum(false), holdingdenom(false) {}

//damageLP::damageLP(damageLP & copy_from_me): lp(new LP(*copy_from_me.lp)),slotMap(copy_from_me.slotMap),unitMap(copy_from_me.unitMap),defenderMap(copy_from_me.defenderMap),Ncol(copy_from_me.Ncol),cols(copy_from_me.cols) {}

void damageLP::insert( const map_location src, const map_location dst, map_location target)
{
#ifdef MCLP_DEBUG
    DBG_AI << "damageLP::insert Ncol = " << Ncol << std::endl;
#endif
    if (lp) {
       ERR_AI << "ERROR: Tried to insert to a damageLP after makelp." << std::endl;
       return;
    }
    col_ptr ptr = cols.insert(cols.end(),++Ncol);
    slotMap.insert(std::make_pair<const map_location,col_ptr> (dst, ptr));
    unitMap.insert(std::make_pair<const map_location,col_ptr> (src, ptr)); 
    defenderMap.insert(std::make_pair<map_location,col_ptr> (target, ptr)); 
}

void ctkLP::insert(map_location src, map_location dst)
{
//#ifdef MCLP_DEBUG
    DBG_AI << "ctkLP::insert Ncol = " << Ncol << " *cols.begin() = " << *(cols.begin()) << std::endl;
//#endif
    if (lp) {
       ERR_AI << "ERROR: Tried to insert to a ctkLP after makelp." << std::endl;
       return;
    }
    col_ptr ptr = cols.insert(cols.end(),++Ncol);
    slotMap.insert(std::make_pair<const map_location,col_ptr> (dst, ptr));
    unitMap.insert(std::make_pair<const map_location,col_ptr> (src, ptr)); 
}

void damageLP::outputMapsAndCols()
{
//    std::stringstream s;
//    s = DBG_AI;
/*    DBG_AI << "Output maps and cols:" << std::endl;
    typedef std::multimap<const map_location, col_ptr>::iterator map_itor;
    map_itor it1,it2;
    bool done = true;
    it1 = slotMap.begin();
    it2 = unitMap.begin();
    if (it1 != slotMap.end()) { done = false; }
    if (it2 != unitMap.end()) { done = false; }
    while (!(done==true))
    {
        done = true;
        if (it1 != slotMap.end())
        { DBG_AI << it1->first << ": " << *(it1->second) << "\t\t"; it1++; done = false;}   
        else 
        {DBG_AI << "      " << ": " << "  " << "\t\t";}        
        if (it2 != unitMap.end())
        { DBG_AI << it2->first << ": " << *(it2->second) << "\n"; it2++; done = false;}   
        else 
        {DBG_AI << "      " << ": " << "  " << "\n";}
    }                                */                                                                               
//    DBG_AI << s.str() << std::endl;
}   

void ctkLP::outputMapsAndCols()
{
//    std::stringstream s;
//    s = DBG_AI;
    return;
    DBG_AI << "Output maps and cols:" << std::endl;
    typedef std::multimap<const map_location, col_ptr>::iterator map_itor;
    map_itor it1,it2;
    bool done = true;
    it1 = slotMap.begin();
    it2 = unitMap.begin();
    if (it1 != slotMap.end()) { done = false; }
    if (it2 != unitMap.end()) { done = false; }
    while (!(done==true))
    {
        done = true;
        if (it1 != slotMap.end())
        { DBG_AI << it1->first << ": " << *(it1->second) << "\t\t"; it1++; done = false;}   
        else 
        {DBG_AI << "      " << ": " << "  " << "\t\t";}        
        if (it2 != unitMap.end())
        { DBG_AI << it2->first << ": " << *(it2->second) << "\n"; it2++; done = false;}   
        else 
        {DBG_AI << "      " << ": " << "  " << "\n";}
    }                               
//    DBG_AI << s.str() << std::endl;
}   


#define remove_X(X) remove_ ## X (const map_location loc)                                                           \
{                                                                                                                   \
    DBG_AI << "called remove_" << #X << " !! NOTE: THIS IS BUGGED RIGHT NOW AND NEEDS A REDISGN " << std::endl;     \
    DBG_AI << "Ncol = " << Ncol << " , lp->end() = " << lp->end() << std::endl;\
    typedef std::multimap<const map_location, col_ptr>::iterator map_itor; \
    map_itor it;\
    std::pair<map_itor,map_itor> range; \
    std::priority_queue<int> to_die;                                                                                \
    int temp = 0;                                                                                                   \
    for (col_ptr ptr = cols.begin(); ptr != cols.end(); ++ptr)                                                      \
    {                                                                                                               \
        (*ptr)=++temp;                                                                                              \
    }                                                                                                               \
    outputMapsAndCols();\
    DBG_AI << "initial condition has Ncol = " << Ncol << " and temp = " << temp << " and size = " << cols.size() << std::endl; \
    for (range = X ## Map.equal_range(loc); range.first != range.second; ++range.first)                             \
    {                                                                                                               \
        temp = *(range.first->second);                                                                              \
        DBG_AI << "to_die.push(" << temp << ");" << std::endl;              \
        assert(0 < temp);                                                                                           \
        assert(temp <= Ncol);                                                                                       \
        to_die.push(temp);                                                                                          \
    }                                                                                                               \
    while (!to_die.empty())                                                                                         \
    {                                                                                                               \
        lp->delete_col(to_die.top());                                                                               \
        cols.remove(to_die.top()); \
        to_die.pop();                                                                                               \
        Ncol--;                                                                                                     \
    }                                                                                                               \
    X ## Map.erase(loc);                                                                                            \
    temp = 0;                                                                                                       \
    for (col_ptr ptr = cols.begin(); ptr != cols.end(); ++ptr)                                                      \
    {                                                                                                               \
        DBG_AI << " *ptr was " << *ptr << std::endl;\
        (*ptr)=++temp;                                                                                              \
        DBG_AI << " *ptr = " << *ptr << std::endl;\
    }                                                                                                               \
    DBG_AI << "postcondition has Ncol = " << Ncol << " and temp = " << temp << " and lp->end() = " << lp->end() << std::endl; \
    outputMapsAndCols();\
    for (col_ptr ptr = cols.begin(); ptr != cols.end(); ++ptr)                                                      \
    {                                                                                                               \
        DBG_AI << " *ptr = " << (*ptr) << std::endl;                                                                \
    }                                                                                                               \
    assert(Ncol == temp);                                                                                           \
}
/*        DBG_AI << "delete_col(" << to_die.top() << ");" << std::endl;       \

//    for (range = X ## Map.equal_range(loc); range.first != range.second; ++range.first)                             \
    {                                                                                                               \
    DBG_AI << "about to erase: cols.size = " << cols.size()  << std::endl;\
        DBG_AI << "cols.erase(" << *(range.first->second) << ");" << std::endl;              \
        cols.erase(range.first->second);                                                                            \
    DBG_AI << "finished erasing: cols.size = " << cols.size()  << std::endl;\
    }                                                                                                               \
*/
    //If the return of equal range were guaranteed to be sorted wrt value we could use this:                        
    //But according to standard we cannot so we throw in a heap first ><
    //
    //for (range = slotMap.equal_range(dst); range.second != range.first; --range.second)
    //{
    //    temp = *(range.second->second);
    //    assert(0 < temp);
    //    assert(temp <= Ncol);
    //    LP->delete_col(temp);
    //    Ncol--;
    //}

void damageLP::remove_X(slot)
void damageLP::remove_X(unit)
void ctkLP::remove_X(slot)
void ctkLP::remove_X(unit)

unsigned char damageLP::remove_col(fwd_ptr ptr)
{
    unsigned char ret;
    ret = lp->delete_col(*ptr);

#ifdef MCLP_DEBUG
    if (ret != LP_SOLVE_TRUE) {
       ERR_AI << "damageLP: Error removing column: " << *ptr << ". Aborted." << std::endl;
       return ret;
    }
#endif
    
    fwd_ptr temp = cols.erase(ptr);
    while (temp != cols.end())
    {
        (*temp)--;
        ++temp;
    }
    Ncol--;
    return ret;
}

unsigned char ctkLP::remove_col(fwd_ptr ptr)
{
    unsigned char ret;
    ret = lp->delete_col(*ptr);
#ifdef MCLP_DEBUG
    if (ret != LP_SOLVE_TRUE) {
       ERR_AI << "ctkLP: Error removing column: " << *ptr << ". Aborted." << std::endl;
       return ret;
    }
#endif

    fwd_ptr temp = cols.erase(ptr);
    while (temp != cols.end())
    {
        (*temp)--;
        ++temp;
    }
    Ncol--;
    return ret;
}

void damageLP::make_lp()
{
#ifdef MCLP_DEBUG
    DBG_AI << "damageLP::make_lp Ncol = " << Ncol << std::endl;
#endif

    //LP new_LP(Ncol); //this makes it on the stack... bad I think
    //lp = & new_LP;
    lp.reset(new LP(Ncol));

    DBG_AI << "damageLP::make_lp adding constraints now" << std::endl;

    lp->rows_LE_1<const map_location> (&slotMap);
#ifdef MCLP_DEBUG
    DBG_AI << "added slot constraints" << std::endl;
#endif
    lp->rows_LE_1<const map_location>(&unitMap);
#ifdef MCLP_DEBUG
    DBG_AI << "added unit constraints" << std::endl;
#endif
    lp->finishRows(); // this will be slow but doing it for now.
    //bool_ptr = cols.begin();
}

void ctkLP::make_lp()
{
#ifdef MCLP_DEBUG
    DBG_AI << "ctkLP::make_lp Ncol = " << Ncol << std::endl;
#endif
    made = true;
    //FracLP new_LP(Ncol); //this make it on the stack... bad I think
    //lp = & new_LP;
    lp.reset(new FracLP(Ncol));

    if (holdingnum) {lp->set_obj_num_constant(holding_num); holdingnum = false; DBG_AI << "Added held num value.." << std::endl;}
    if (holdingdenom) {lp->set_obj_denom_constant(holding_denom); holdingdenom = false; DBG_AI << "Added held denom value.." << std::endl;}

    lp->rows_LE_1<const map_location>(&slotMap);
#ifdef MCLP_DEBUG
    DBG_AI << "added slot constraints" << std::endl;
#endif
    lp->rows_LE_1<const map_location>(&unitMap);
#ifdef MCLP_DEBUG
    DBG_AI << "added unit constraints" << std::endl;
#endif
    lp->finishRows(); // this will be slow but doing it for now.
    //bool_ptr = cols.begin();
}

unsigned char damageLP::set_obj(fwd_ptr ptr, REAL r)
{
    //if (ptr == bool_ptr) { lp->set_boolean(*ptr); bool_ptr++; }
    return lp->set_obj(*ptr,r);
}

unsigned char ctkLP::set_obj_num(fwd_ptr ptr, REAL r)
{
    //if (ptr == bool_ptr) { lp->set_boolean(*ptr); bool_ptr++; }
    return lp->set_obj_num(*ptr,r);
}

unsigned char ctkLP::set_obj_denom(fwd_ptr ptr, REAL r)
{
    return lp->set_obj_denom(*ptr,r);
}

unsigned char ctkLP::set_obj_num_constant(REAL r)
{
    if (made) {
#ifdef MCLP_DEBUG
    DBG_AI << "cktLP::Set num constant " << r << std::endl;
#endif
    return lp->set_obj_num_constant(r);
    } else {
        DBG_AI << "Now holding num = " << r << std::endl;
        holdingnum = true;
        holding_num = r;
        return LP_SOLVE_TRUE;
    }
}

unsigned char ctkLP::set_obj_denom_constant(REAL r)
{
    if (made) {
#ifdef MCLP_DEBUG
    DBG_AI << "cktLP::Set denom constant " << r << std::endl;
#endif
    return lp->set_obj_denom_constant(r);
    } else {
        DBG_AI << "Now holding denom = " << r << std::endl;
        holdingdenom = true;
        holding_denom = r;
        return LP_SOLVE_TRUE;
    }
}

unsigned char damageLP::set_col_name(fwd_ptr ptr, char *str)
{
    return lp->set_col_name(*ptr,str);
}

unsigned char ctkLP::set_col_name(fwd_ptr ptr, char *str)
{
    return lp->set_col_name(*ptr,str);
}

unsigned char damageLP::set_boolean(fwd_ptr ptr)
{
    return lp->set_boolean(*ptr);
}
/*    if (bool_ptr != cols.end())
    {
        ERR_AI << "damageLP: Going make to make all vars boolean..." << std::endl;
        for (bool_ptr = cols.begin(); bool_ptr != cols.end(); bool_ptr++)
        {
            lp->set_boolean(*bool_ptr);
        }
    }*/
int damageLP::solve()
{
    return lp->solve();
}


unsigned char ctkLP::set_boolean(fwd_ptr ptr)
{
    return lp->set_boolean(*ptr);
}
/*    if (bool_ptr != cols.end())
    {
        ERR_AI << "ctkLP: Going make to make all vars boolean..." << std::endl;
        ERR_AI << "ctkLP: WARNING: Some denominator values could be indetermined..." << std::endl;
        for (bool_ptr = cols.begin(); bool_ptr != cols.end(); bool_ptr++)
        {
            lp->set_boolean(*bool_ptr);
        }
    }*/

int ctkLP::solve()
{
    return lp->solve();
}

REAL damageLP::get_obj()
{
    return lp->get_obj();
}

REAL ctkLP::get_obj()
{
    return lp->get_obj();
}

REAL damageLP::get_obj_without(map_location unit, map_location slot)
{
    typedef std::multimap<const map_location, col_ptr>::iterator map_itor; 
    map_itor it;
    std::pair<map_itor,map_itor> range; 
    std::priority_queue<int> to_die;                                                                                

    boost::shared_ptr<LP> templp(new LP(*lp));
    int temp = 0;                                                                                                   
    for (range = unitMap.equal_range(unit); range.first != range.second; ++range.first)                             
    {                                                                                                               
        temp = *(range.first->second);                                                                              
        //DBG_AI << "to_die.push(" << temp << ");" << std::endl;              
        assert(0 < temp);                                                                                           
        assert(temp <= Ncol);                                                                                       
        to_die.push(temp);                                                                                          
    }                                                                                                               
    for (range = slotMap.equal_range(slot); range.first != range.second; ++range.first)                             
    {                                                                                                               
        temp = *(range.first->second);                                                                              
        //DBG_AI << "to_die.push(" << temp << ");" << std::endl;              
        assert(0 < temp);                                                                                           
        assert(temp <= Ncol);                                                                                       
        to_die.push(temp);                                                                                          
    }                                                                                                               
    temp = -1000000;
    while (!to_die.empty())                                                                                         
    {              
        if(to_die.top()!=temp) { //don't delete same column twice
            templp->delete_col(to_die.top());
            temp = to_die.top();
        }
        to_die.pop();                                                                                               
    } 
    templp->solve();
    return templp->get_obj();
}

REAL damageLP::get_var(fwd_ptr ptr)
{
    return lp->get_var(*ptr);
}

REAL ctkLP::get_var(fwd_ptr ptr)
{
    return lp->get_var(*ptr);
}

bool ctkLP::var_gtr(fwd_ptr ptr, REAL eps)
{
    return lp->var_gtr(*ptr, eps);
}

unsigned char damageLP::write_lp(char * file)
{
    return lp->write_lp(file);
}

unsigned char ctkLP::write_lp(char * file)
{
    return lp->write_lp(file);
}

//*************************************************************
// LP_AI_TACTICS
//*************************************************************

LP_AI_TACTICS::LP_AI_TACTICS() {}
void LP_AI_TACTICS::insert( map_location src, map_location dst, map_location target, int weapon) {}
void LP_AI_TACTICS::remove_slot( map_location dst) {}
void LP_AI_TACTICS::remove_unit( map_location unit) {}

int begin() {return 0;}// { return cols.begin(); }
int end() {return 0;} //{ return cols.end(); }

int LP_AI_TACTICS::solve() {return 0;}

REAL LP_AI_TACTICS::get_obj() {return 0;}
REAL LP_AI_TACTICS::get_var(int) {return 0;}
bool LP_AI_TACTICS::var_gtr(int, REAL) {return 0;}

unsigned char LP_AI_TACTICS::write_lp (char *) {return 0;}
    

//*************************************************************
// END Implementation of LP wrappers
//*************************************************************
}
