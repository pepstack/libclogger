/**
 * colib_api.h
 */
#ifndef _COLIB_API_H_
#define _COLIB_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <common/memapi.h>
#include <common/unitypes.h>

#ifndef coiid_t
 typedef ub8 coiid_t;
#endif


#define IID_CompntBase      ((coiid_t) 0x00000000)


/**
 * Component library (XXX_colib.so) must implement the following 4 APIs
 */
typedef struct CompntLibBase
{
    const char * (*colibVersion)();
    int (*colibInit)(void *, void**);
    void (*colibUninit)(void *);
    int (*colibCoCreate)(void *,coiid_t, void **);
} ICompntLibBase;


/**
 * Component must inherit ICompntBase from CompntBaseClass
 */
typedef struct CompntBaseClass
{
    coiid_t iid;
    void *libstub;
    void (*codestroy)(void *cobase);
} ICompntBase;


static void CompntClassFree(void *coaddr)
{
    ICompntBase *base = (ICompntBase*) coaddr;
    base--;
    mem_free(base);
}


/**
 * CompntCreate(NULL, IID_PolygonCompnt, sizeof(PolygonCompnt), PolygonCompntDtor);
 */
static void * CompntClassCreate(size_t cobodysize, coiid_t iid, void *libstub, void (*codestroy)(void *))
{
    ICompntBase *cobase = (ICompntBase *) mem_alloc_zero(1, sizeof(ICompntBase) + cobodysize);
    cobase->iid = iid;
    cobase->libstub = libstub;
    cobase->codestroy = codestroy;
    return (void *) &cobase[1];
}


#if defined(__cplusplus)
}
#endif

#endif /* _COLIB_API_H_ */
