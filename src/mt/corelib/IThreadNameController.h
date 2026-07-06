#pragma once
#include <string>
#ifdef _WIN32
#else
#include <pthread.h>
#endif

/// interface tool to debug mutex deadlocks etc

struct mutex_inspector_entry;
class IThreadNameController
{
public:
    virtual void add_LK(const pthread_t&key, const std::string& val)=0;
    virtual void remove_LK(const pthread_t&key)=0;
    virtual void remove_LK(const std::string& val)=0;
    virtual std::string getName_LK(const pthread_t& pt)=0;
    virtual ~IThreadNameController() {}
    virtual std::string dump_mutex_inspector(const pthread_t &pt)=0;
    virtual std::string dump_mutex_inspectors()=0;
    virtual void print_term(int signum)=0;
    virtual void push_mi(const pthread_t& pt, mutex_inspector_entry* e)=0;
    virtual void pop_mi(const pthread_t& pt)=0;
};

