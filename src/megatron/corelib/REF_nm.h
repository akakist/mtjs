#pragma once

#ifdef _MSC_VER
#include <time.h>
#endif
#include <atomic>
#include <stdio.h>
/// base class and template of smart pointer with refcount, which can catch pointer many times.
///
#define PRIVATEMUTEX
template < class T > class REF_getterNM;
class RefcountableNM
{
    friend class REF_getterNM < RefcountableNM >;
    RefcountableNM (const RefcountableNM &);	// Not defined  to prevent usage
    RefcountableNM & operator= (const RefcountableNM &);	// Not defined  to prevent usage

public:
#ifdef PRIVATEMUTEX
//    Mutex __privateMuteX;
#endif
    int __Ref_Count;
    RefcountableNM (): __Ref_Count(0) { }

    int get_ref_count () const
    {
        return __Ref_Count;
    };
    virtual ~ RefcountableNM ()
    {
        if(__Ref_Count!=0)
        {
            printf("destructor with __Ref_Count!=0\n");
        }
    }
};

///      Smart pointer template
template < class T > class REF_getterNM
{

public:
    T * ___ptr;
    inline void increfcount () const
    {
        if (___ptr)
        {
            ++___ptr->__Ref_Count;
        }
    }
    inline void decrefcount ()
    {
        {
            if (___ptr)
            {
                auto prev=___ptr->__Ref_Count;
                --___ptr->__Ref_Count;
                if (prev==1) {
                    {
                        delete ___ptr;
                        ___ptr = NULL;
                    }
                }
            }
        }
    }

    inline bool operator < (const REF_getterNM & l) const
    {
        return ___ptr < l.___ptr;
    }
    inline bool valid() const
    {
        return ___ptr!=NULL;
    }
    inline REF_getterNM (const T * p):___ptr ((T *) p)
    {
        increfcount ();
    }
    inline REF_getterNM (const REF_getterNM < T > &bcg):___ptr (bcg.___ptr)
    {
        increfcount ();
    }

    ~REF_getterNM ()
    {
        decrefcount ();
    }
    bool operator== (const REF_getterNM & bcg) const
    {
        return ___ptr == bcg.___ptr;
    }
#ifndef _MSC_VER
    inline REF_getterNM & operator = (const REF_getterNM & bcg) const
    {
        if (this != &bcg)
        {
            bcg.increfcount ();
            decrefcount ();
            ___ptr = bcg.___ptr;
        }
        return *this;
    }
#endif
    inline REF_getterNM & operator = (const REF_getterNM & bcg)
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
        return ___ptr;
    };
};
