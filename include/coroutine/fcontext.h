#ifndef _FCONTEXT_H_
#define _FCONTEXT_H_

#include <cstddef>
using fcontext_t = void*;


struct transfer_t {
    fcontext_t  fctx;
    void    *   data;
};

extern "C" transfer_t jump_fcontext( fcontext_t const to, void * vp);
extern "C" fcontext_t make_fcontext( void * sp, std::size_t size, void (* fn)( transfer_t) );


#endif // _FCONTEXT_H_