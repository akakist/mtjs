#include "IUtils.h"
#include "quickjs.h"
// #include "json/json.h"
#ifdef DEBUG
extern "C"
void memctl_add_malloc(void *p, const char* s)
{
    iUtils->add_malloc(p,s);
}
extern "C"
void memctl_add_realloc(void *p_old, void *p_new)
{
    iUtils->add_realloc(p_old,p_new);
}
extern "C"
void memctl_remove_malloc(void *p)
{
    iUtils->remove_malloc(p);
}
extern "C"
void memctl_add_object( JSValue v, const char *s)
{
    if(JS_VALUE_HAS_REF_COUNT(v))
    {
        memctl_add_malloc(JS_VALUE_GET_PTR(v),s);
    }
}

#endif