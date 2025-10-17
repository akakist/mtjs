#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "IUtils.h"
#include "commonError.h"
#include "mutexInspector.h"
#ifdef _WIN32
#include <io.h>
#endif
#ifndef _MSC_VER
#include <unistd.h>
#endif

/// stacked FILE container, automatically closed on destructor
class st_FILE
{

public:
    FILE *f;

    st_FILE (const char* pn, const char* mode)
    {
        f = fopen(pn, mode);

        if (f == NULL)
        {
            throw CommonError("fopen failed with %s errno %d (%s) %s",pn,errno,strerror(errno),_DMI().c_str());
        }
    }

    st_FILE (const std::string& pn, const char* mode)
    {
        f = fopen(pn.c_str(), mode);

        if (f == NULL)
        {
            throw CommonError("fopen failed with %s errno %d (%s)",pn.c_str(),errno,strerror(errno));
        }
    }

    /** destructor автоматически закрывает файл
    */
    virtual  ~st_FILE ()
    {
        if (f)
        {
            fclose(f);
        }

    }
};

/// stacked FD container, automatically closed on destructor
class st_FD
{
public:
    int fd;
    st_FD(int _fd):fd(_fd) {}
    ~st_FD()
    {
        if(fd>0)
        {
            ::close(fd);
        }
    }
};
