#pragma once
/*
enum PKT_TYPE
{
 PKT_DATA,
 PKT_END,
 PKT_ERROR,
 PKT_META,
};*/

struct Stream: public Refcountable
{
    virtual std::string read(size_t n)=0; // return size==0 means stream end
    virtual bool rd_hasContentLength()=0;
    virtual size_t rd_contentLength()=0;

    virtual void write(const std::string& cmd,const  std::string & s)=0;

    // virtual ~Stream()=default;
    Stream(const char* name):classname(name) {

        DBG(iUtils->mem_add_ptr("Stream",this));

    }
    ~Stream()
    {
        DBG(iUtils->mem_remove_ptr("Stream",this));

    }

    REF_getter<Stream> piped=nullptr;
    void pipe(const REF_getter<Stream> &p)
    {
        piped=p;
    }
    const char * classname;

};
