#pragma once
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#if !defined __MOBILE__
#if !defined  __ANDROID_API__ && !defined __FreeBSD__
#include <sys/timeb.h>
#endif
#endif
#ifdef _WIN32
#include <pthread.h>
#endif

#define _nano_ 1000000000


/// C++ wrapper for timespec

struct mtimespec: public timespec
{
    mtimespec(long sec,long nsec);
    mtimespec() ;
};
void nanotime(timespec* t);

int operator<(const mtimespec& a,const mtimespec& b);
int operator<=(const mtimespec& a,const mtimespec& b);
mtimespec operator+(const mtimespec& a,const mtimespec& b);
mtimespec operator-(const mtimespec& a,const mtimespec& b);
