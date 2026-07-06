#pragma once

#include <map>
#include <string>
#include <deque>
#include "IThreadNameController.h"


class ThreadNameControllerImpl;
class ThreadNameController:public IThreadNameController
{
public:
    ThreadNameControllerImpl* impl;
    ThreadNameController();
    ~ThreadNameController();
    void add_LK(const pthread_t &key, const std::string& val);
    void remove_LK(const pthread_t&key);
    void remove_LK(const std::string& val);
    std::string getName_LK(const pthread_t& pt);
    std::string getName(const pthread_t& pt);
    std::string dump_mutex_inspector(const pthread_t&pt);
    std::string dump_mutex_inspectors();
    void print_term(int signum);
    void push_mi(const pthread_t& pt, mutex_inspector_entry* e);
    void pop_mi(const pthread_t& pt);
};

