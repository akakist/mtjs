#pragma once
#include "httpConnection.h"
#include "common/eventEmitter.h"
#include "common/jsscope.h"

struct HTTP_ResponseP: public Refcountable
{
    HTTP::Response resp;
    HTTP_ResponseP(const REF_getter<HTTP::Request>&r): resp(r)
    {
    }
    ~HTTP_ResponseP()
    {
    }
};

struct HTTP_RequestP: public Refcountable
{
    REF_getter<HTTP::Request> req;
    HTTP_RequestP(const REF_getter<HTTP::Request>&rq): req(rq)
    {
    }
    ~HTTP_RequestP()
    {
    }

};