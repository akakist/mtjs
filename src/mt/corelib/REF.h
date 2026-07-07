#pragma once

#ifdef _MSC_VER
#include <time.h>
#endif
#include <atomic>
#include <stdio.h>
// #include "mutexInspector.h"
/// base class and template of smart pointer with refcount, which can catch pointer many times.
#ifdef MEMLEAK_CHECK
void inc_ptr(const char* s);
void dec_ptr(const char* s);
#endif
void throw_deref_nullptr();
template < class T > class REF_getter;
class Refcountable
{
    friend class REF_getter < Refcountable >;
    Refcountable (const Refcountable &);	// Not defined  to prevent usage
    Refcountable & operator= (const Refcountable &);	// Not defined  to prevent usage

    const char* ___nm;
public:
    std::atomic<int> __Ref_Count;
    Refcountable (const char* nm): __Ref_Count(0),___nm(nm)
    {
#ifdef MEMLEAK_CHECK
        inc_ptr(___nm);
#endif
    }
    int get_ref_count () const
    {
        return __Ref_Count;
    };
    virtual ~ Refcountable ()
    {
#ifdef MEMLEAK_CHECK
        dec_ptr(___nm);
#endif
        if(__Ref_Count!=0)
        {
            printf("destructor with __Ref_Count!=0\n");
        }
    }
};

///      Smart pointer template
template < class T > class REF_getter
{

public:
    T * ___ptr;
    void increfcount () const
    {
        if (___ptr)
        {
            std::atomic_fetch_add(&___ptr->__Ref_Count,1);
        }
    }
    void decrefcount ()
    {
        bool need_destroy = false;
        {
            if (___ptr)
            {
                auto prev=std::atomic_fetch_sub(&___ptr->__Ref_Count,1);
                if (prev==1)
                {
                    delete ___ptr;
                    ___ptr = NULL;
                }
            }
        }
    }

    bool operator < (const REF_getter & l) const
    {
        return ___ptr < l.___ptr;
    }
    bool valid() const
    {
        return ___ptr!=NULL;
    }
    REF_getter():___ptr (NULL)
    {
    }
    REF_getter (const T * p):___ptr ((T *) p)
    {
        increfcount ();
    }
    REF_getter (const REF_getter < T > &bcg):___ptr (bcg.___ptr)
    {
        increfcount ();
    }
    ~REF_getter ()
    {
        if(___ptr)
            decrefcount ();
    }
    bool operator== (const REF_getter & bcg) const
    {
        return ___ptr == bcg.___ptr;
    }
#ifndef _MSC_VER
    REF_getter & operator = (const REF_getter & rh) const
    {
        if (this != &rh)
        {
            rh.increfcount ();
            decrefcount ();
            ___ptr = rh.___ptr;
        }
        return *this;
    }
#endif
    REF_getter & operator = (const REF_getter & bcg)
    {
        if (this != &bcg)
        {
            bcg.increfcount ();
            decrefcount ();
            ___ptr = bcg.___ptr;
        }
        return *this;
    }
    inline  T* operator -> () const
    {
        #ifdef DEBUG
        if(!___ptr)
        {
            throw_deref_nullptr();
        }
        #endif
        return ___ptr;
    };
    inline  T* get() const
    {
        #ifdef DEBUG
        if(!___ptr)
        {
            throw_deref_nullptr();
        }
#endif
        return ___ptr;
    };
};


