#pragma once
#include <string>
#include <vector>
#include <postgresql/libpq-fe.h>
#ifdef _WIN32
#include <time.h>
#endif
#include "DBH.h"
#include "unknown.h"
#include "listenerSimple.h"
#define INACTIVE_CONNECTION_DROP_TIMEOUT_SEC 60
#define max_reconnects 20

struct mysqlConf
{
    int max_connections;
    std::string configString;

};

class DBH_postgres:public DBH
{
public:
    virtual void exec(const QUERY &query);
    virtual REF_getter<QueryResult> exec(const QUERY &);
    virtual std::string escape(const std::string&);

    DBH_postgres(const mysqlConf&c);
    virtual ~DBH_postgres();
    time_t getLastQueryTime();


    void exec(const std::string &);
    REF_getter<QueryResult> exec(const std::string &);

    void makeConnection();
    bool bIsValid();
private:
    PGconn *dbh;
    bool bIsLost;
    mysqlConf cnf;
    time_t m_lastQueryTime;

protected:
};
class DBH_source_postgres:public UnknownBase, public ListenerSimple, public DBH_source
{
public:
    REF_getter<DBH> get();
    void unget(const REF_getter<DBH>&);
    static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IConfigObj*);
    DBH_source_postgres(const SERVICE_id& _id, const std::string&  nm,IConfigObj*);

    int max_connections;

    Mutex mutex;
    std::deque<REF_getter<DBH> > container_MX;

    bool handleEvent(const REF_getter<Event::Base>&)
    {
        return true;
    }

    mysqlConf cfg;

};
