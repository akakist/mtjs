#pragma once
#include "quickjs.h"
#ifdef __cplusplus
extern "C"
{
#endif
void memctl_add_malloc(void *p, const char* s);
void memctl_add_realloc(void *p_old, void *p_new);
void memctl_remove_malloc(void *p);
void memctl_add_object(JSValue v, const char *s);
#ifdef __cplusplus
}
#endif

