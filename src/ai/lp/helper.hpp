#ifndef AI_LP_HELPER_HPP_INCLUDED
#define AI_LP_HELPER_HPP_INCLUDED

#include "MCLP_FLAGS.hpp"

#include <map>
#include <list>
#include <boost/shared_ptr.hpp>

#ifndef REAL 
   #define REAL double
#endif

class map_location;
class LP;
class FracLP;

namespace ai {

typedef std::list<int>::iterator col_ptr;

//why no forward iterator
typedef std::list<int>::iterator fwd_ptr;

/** A class that manages an LP which estimates damage potential for the army. **/
//You must iterate make all insertions before calling make_lp.
//You must then iterate a forward pointer and set_obj to make all variables boolean.
//If you fail to do this the class will iterate again before you solve to make sure this happens.
//Best not to delete anything until after all vars have been made boolean.

class damageLP {
public:
    damageLP();
    //~damageLP() { DBG_AI << "~damageLP();" << std::endl; } // if (lp != NULL) { delete(lp); } }

    void insert( map_location src, map_location dst, map_location def);
    void make_lp();
    void remove_slot(map_location dst);
    void remove_unit(map_location unit);
    //including remove_defender makes no sense, you would have to update the new moves that are possible.

    fwd_ptr begin() {return cols.begin(); }
    fwd_ptr end() {return cols.end(); }
    
    unsigned char remove_col( fwd_ptr );

    unsigned char set_boolean ( fwd_ptr );
    unsigned char set_obj (fwd_ptr, REAL );
    unsigned char set_col_name (fwd_ptr, char *);

    unsigned char solve();

    REAL get_obj();
    REAL get_var(fwd_ptr);

    unsigned char write_lp (char *);

    typedef fwd_ptr iterator;

private:
    boost::shared_ptr<LP> lp;

    //Each attacker destination "slot", and each attacker, becomes a row in LP since each unit and slot can be used only once.
    std::multimap<const map_location, col_ptr> slotMap; 
    std::multimap<const map_location, col_ptr> unitMap;
    std::multimap<map_location, col_ptr> defenderMap; // No real use atm but may be useful for updates later.

    int Ncol;

    //Rather than store ints for columns, store pointers to this list so we can delete easily.
    std::list<int> cols;
};

/** A class that manages an LP which estimates opt ctk for a target. **/
//You must iterate make all insertions before calling make_lp.
//You must then iterate a forward pointer and set_boolean to make all variables boolean.
//If you fail to do this the class will iterate again before you solve to make sure this happens.
//Best not to remove anything until after all vars have been made boolean.

class ctkLP {
public:
    ctkLP(const map_location * ml);
    ctkLP(boost::shared_ptr<const map_location> ml);
    //~ctkLP() { DBG_AI << "~ctkLP();" << std::endl; } //if (lp) {} else { delete(lp); } }

    void insert( map_location src, map_location dst);
    void make_lp();
    void remove_slot( map_location dst);
    void remove_unit( map_location unit); 

    fwd_ptr begin() { return cols.begin(); }
    fwd_ptr end() { return cols.end(); }

    unsigned char remove_col( fwd_ptr );

    unsigned char set_boolean ( fwd_ptr );

    unsigned char set_obj_num(fwd_ptr ptr, REAL );
    unsigned char set_obj_denom(fwd_ptr ptr, REAL );
    unsigned char set_obj_num_constant(REAL );
    unsigned char set_obj_denom_constant(REAL );
    unsigned char set_col_name (fwd_ptr, char *);

    unsigned char solve();

    REAL get_obj();
    REAL get_var(fwd_ptr);
    bool var_gtr(fwd_ptr, REAL);

    unsigned char write_lp (char *);
    
    typedef fwd_ptr iterator;
    boost::shared_ptr<const map_location> defender; //This was private but i make it public now for debugging output


private:
    boost::shared_ptr<FracLP> lp;

    //Each attacker destination "slot", and each attacker, becomes a row in LP since each unit and slot can be used only once.
    std::multimap<const map_location, col_ptr> slotMap;
    std::multimap<const map_location, col_ptr> unitMap;

    int Ncol;

    //Rather than store ints for columns, store pointers to this forward list so we can delete easily.
    std::list<int> cols;

    REAL holding_num;
    REAL holding_denom;
    bool made;
    bool holdingnum;
    bool holdingdenom;
//    fwd_ptr bool_ptr;
};

}

#endif
