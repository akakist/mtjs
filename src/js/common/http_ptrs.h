#pragma once
#include "httpConnection.h"
#include "common/eventEmitter.h"
#include "common/jsscope.h"

struct HTTP_ResponseP: public Refcountable
{
        
    HTTP::Response resp;
    HTTP_ResponseP(const REF_getter<HttpContext>&r): Refcountable("HTTP_ResponseP"), resp(r)
    {
    }
    ~HTTP_ResponseP()
    {
    }
};

struct HTTP_RequestP: public Refcountable
{
        
    REF_getter<HttpContext> req;
    HTTP_RequestP(const REF_getter<HttpContext>&rq): Refcountable("HTTP_RequestP"),
    req(rq)
    {
    }
    ~HTTP_RequestP()
    {
    }

};