#pragma once
#include "httpConnection.h"
#include "common/eventEmitter.h"
#include "common/jsscope.h"

struct HTTP_ResponseP: public Refcountable
{
    HTTP::Response resp;
    HTTP_ResponseP(const REF_getter<HTTP::Request>&r): resp(r)
    {
        DBG(iUtils->mem_add_ptr("HTTP_ResponseP",this));
    }
    ~HTTP_ResponseP()
    {
        DBG(iUtils->mem_remove_ptr("HTTP_ResponseP",this));
    }
};

struct HTTP_RequestP: public Refcountable
{
    REF_getter<HTTP::Request> req;
    HTTP_RequestP(const REF_getter<HTTP::Request>&rq): req(rq)
    {
        DBG(iUtils->mem_add_ptr("HTTP_RequestP",this));

    }
    ~HTTP_RequestP()
    {
        DBG(iUtils->mem_remove_ptr("HTTP_RequestP",this));
    }

};