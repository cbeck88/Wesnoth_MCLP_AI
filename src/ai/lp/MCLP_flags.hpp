#ifndef MCLP_FLAGS_HPP
#define MCLP_FLAGS_HPP

//Enable this to save lp files from MCLP for debugging
//#define MCLP_FILEOUT

//Enable this to get full MCLP debugging output
#define MCLP_DEBUG

//Enable to make it set lower bounds on vars using lp_solve::set_lowbo. defaults of >= 0 for all seems fine to me.
//#define LP_SET_LOWBO 

#endif
