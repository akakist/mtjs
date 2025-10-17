#pragma once
#include <string>
#include <deque>
#include <mysql.h>
#ifdef _WIN32
#include <time.h>
#endif
#include "DBH.h"
#include "unknown.h"
#include "listenerSimple.h"
#define INACTIVE_CONNECTION_DROP_TIMEOUT_SEC 60

struct mysqlConf
{
    int max_connections;
    std::string host;
    std::string user;
    std::string passwd;
    std::string database;
    // std::string start_code;
    int port;
    std::string sock;
    unsigned long flag=0;

};

class DBH_mysql:public DBH
{
public:
    // sql like "update, insert"
    void execSimple(const QUERY &);
    REF_getter<QueryResult> exec(const QUERY &);
    REF_getter<QueryResult> exec(const std::string &query);


    std::string escape(const std::string&);

    DBH_mysql();
    virtual ~DBH_mysql();
    time_t getLastQueryTime();
    void execSimple(const std::string &);
    void makeConnection(const mysqlConf& conf);
private:
    bool bIsValid();
    MYSQL *dbh;
    bool bIsLost;

    time_t m_lastQueryTime;

protected:
};
class DBH_source_mysql:public UnknownBase, public ListenerSimple, public DBH_source, public Mutexable
{
    int max_connections;
    mysqlConf cfg;
public:
    REF_getter<DBH> get();
    void unget(const REF_getter<DBH>&);
    static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa);
    DBH_source_mysql(const SERVICE_id& _id, const std::string&, IInstance* ifa);
    ~DBH_source_mysql();

    bool  handleEvent(const REF_getter<Event::Base>&e);
    void deinit() {}


    std::deque<REF_getter<DBH> > container_MX;

};
