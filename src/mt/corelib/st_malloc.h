#pragma once
#include <stdio.h>
#include "commonError.h"
#if !defined __MACH__ && !defined __FreeBSD__
#include <malloc.h>
#endif
/// stacked malloc, automatically call free() on destructor


struct st_malloc
{
private:
    st_malloc(const st_malloc&);             // Not defined
    st_malloc& operator=(const st_malloc&);  // Not defined
public:
    void *buf;
    st_malloc(size_t size)
    {
        buf = malloc(size + 1);
        if (buf==NULL)throw CommonError("st_malloc failed");
    }
    ~st_malloc()
    {
        if (buf)
        {
            free(buf);
            buf=NULL;
        }
    }

};
