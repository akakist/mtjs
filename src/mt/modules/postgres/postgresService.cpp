#include <stdio.h>
#include "IInstance.h"
#include "DBH.h"
#include "postgresService.h"
#include <string>
#include "commonError.h"
#ifdef _WIN32
#include "compat_win32.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#endif
#include <deque>
#include <stdarg.h>
#ifndef _WIN32
#include <alloca.h>
#endif
#include <unistd.h>
#include <vector>
#include "DBHEvent.h"
#include "queryResult.h"
#include "version_mega.h"
#include "VERSION_id.h"

bool DBH_postgres::bIsValid()
{

    if (!dbh) return false;
    if (bIsLost) return false;
    return true;
}

REF_getter<QueryResult>DBH_postgres::exec(const std::string &query)
{
    QUERY q(query);
    return exec(q);
}

REF_getter<QueryResult>DBH_postgres::exec(const QUERY &query)
{
    m_lastQueryTime=time(NULL);

    REF_getter<QueryResult> result(new QueryResult);

    char *paramValues[query.params.size()];
    int paramLengths[query.params.size()];

    for (size_t i=0; i<query.params.size(); i++)
    {
        logErr2("param %s",query.params[i].c_str());
        paramValues[i]=(char*)query.params[i].data();
        paramLengths[i]=query.params[i].size();
    }

    while (1)
    {
        PGresult *res=PQexecParams(dbh, query.query.c_str(), query.params.size(), NULL, paramValues, paramLengths, NULL, 0);
        //result status
        int rs=PQresultStatus(res);
        if (PQstatus(dbh)!=CONNECTION_OK)  	//connection dead?
        {
            logErr2("DBH_postgres::execSimple() failed with [%s], trying to reconnect in 1 sec...", PQerrorMessage(dbh));
            sleep(1);
            int attempt=0;
            while (attempt < max_reconnects)
            {
                try
                {
                    attempt++;
                    logErr2("Attempting to reconnect #%d", attempt);
                    makeConnection();
                    logErr2("Connected!");
                    break;
                }
                catch (...)
                {
                    throw CommonError("DBH_postgres::execSimple() failed, couldn't reconnect in %d times", max_reconnects);
                }
            }
            continue;
        }
        if (rs==PGRES_NONFATAL_ERROR)  	//notice or warning
        {
            logErr2("DBH_postgres::exec() warning: %s", PQerrorMessage(dbh));
            PQclear(res);
            break;
        }
        if (rs==PGRES_FATAL_ERROR)  	//query failed
        {
            PQclear(res);
            throw CommonError("DBH_postgres::exec() query failed: [%s] with message [%s]", query.query.c_str(), PQerrorMessage(dbh));
        }
        if (rs==PGRES_COMMAND_OK)  	//query ok, no data
        {
            logErr2("if (rs==PGRES_COMMAND_OK)  	//query ok, no data");
            PQclear(res);
            break;
        }
        if (rs==PGRES_TUPLES_OK)  	//query ok, data returned
        {
            logErr2("if (rs==PGRES_TUPLES_OK)  	//query ok, data returned");
            //REF_getter<QueryResult> result(new QueryResult);

            result->numCols=PQnfields(res);
            result->numRows=PQntuples(res);

            logErr2("result->numCols %d result->numRows %d",result->numCols,result->numRows);

            //get field names
            for (size_t i=0; i<result->numCols; i++)
            {
                result->fields[i]=PQfname(res, i);
            }
            //get rows
            for (size_t i=0; i<result->numRows; i++)
            {
                std::vector<std::string> v;
                for (size_t j=0; j<result->numCols; j++)
                {
                    v.push_back(std::string(PQgetvalue(res, i, j), PQgetlength(res, i, j)));
                }

                result->values.push_back(v);
            }
            PQclear(res);
            break;
        }
    }

    return result;

}
void DBH_postgres::execSimple(const std::string &q)
{
    exec(q);
}
void DBH_postgres::execSimple(const QUERY &query)
{
//    return execSimple(q.query);
    char *paramValues[query.params.size()];
    int paramLengths[query.params.size()];

    for (size_t i=0; i<query.params.size(); i++)
    {
        logErr2("param %s",query.params[i].c_str());
        paramValues[i]=(char*)query.params[i].data();
        paramLengths[i]=query.params[i].size();
    }

    while (1)
    {
        m_lastQueryTime=time(NULL);
//        PGresult *res=PQexec(dbh, q.query.c_str());
        PGresult *res=PQexecParams(dbh, query.query.c_str(), query.params.size(), NULL, paramValues, paramLengths, NULL, 0);

        //result status
        int rs=PQresultStatus(res);
        if (PQstatus(dbh)!=CONNECTION_OK)  	//connection dead?
        {
            logErr2("DBH_postgres::execSimple() failed with [%s], trying to reconnect in 1 sec...", PQerrorMessage(dbh));
            sleep(1);
            int attempt=0;
            while (attempt < max_reconnects)
            {
                try
                {
                    attempt++;
                    logErr2("Attempting to reconnect #%d", attempt);
                    makeConnection();
                    logErr2("Connected!");
                    break;
                }
                catch (...)
                {
                    throw CommonError("DBH_postgres::execSimple() failed, couldn't reconnect in %d times", max_reconnects);
                }
            }
            continue;
        }
        if (rs==PGRES_NONFATAL_ERROR)  	//notice or warning
        {
            logErr2("DBH_postgres::execSimple() warning: %s", PQerrorMessage(dbh));
            PQclear(res);
            return;
        }
        if (rs==PGRES_FATAL_ERROR)  	//query failed
        {
            PQclear(res);
            throw CommonError("DBH_postgres::execSimple() query failed: [%s] with message [%s]", query.query.c_str(), PQerrorMessage(dbh));
        }
        if (rs==PGRES_COMMAND_OK)  	//success
        {
            PQclear(res);
            return;
        }
        if (rs==PGRES_TUPLES_OK)  	//success with result
        {
            PQclear(res);
            logErr2("DBH_postgres::execSimple() query executed successfully, but returned result, that does not mean in execSimple.");
            return;
        }
    }

}


std::string DBH_postgres::escape(const std::string& s)
{

    return s;

}
void DBH_postgres::makeConnection()
{

    logErr2("cnf.configString.c_str() %s",cnf.configString.c_str());
    dbh=PQconnectdb(cnf.configString.c_str());
    //postgres_init(dbh);
    if (PQstatus(dbh) != CONNECTION_OK)
    {
        logErr2("Connection to database failed: %s",
                PQerrorMessage(dbh));
        throw CommonError("Connection to database failed: %s",
                          PQerrorMessage(dbh));
    }

}
DBH_postgres::~DBH_postgres()
{

    if (dbh)
    {
        PQfinish(dbh);
        dbh=NULL;
    }
}



DBH_postgres::DBH_postgres(const mysqlConf&c):
    dbh(NULL),bIsLost(false), cnf(c),m_lastQueryTime(time(NULL))
{
}

DBH_source_postgres::DBH_source_postgres(const SERVICE_id& _id, const std::string&  nm,IConfigObj* config):
    UnknownBase(nm),
    ListenerSimple(this,nm,config,_id),
    DBH_source(this),
    max_connections(20)
{
    cfg.max_connections=config->get_int64_t("max_connections",20,"");
    cfg.configString=config->get_string("configString","","");

}
REF_getter<DBH> DBH_source_postgres::get()
{
    //DBH_postgres *dbh=NULL;
    size_t nOpenedConnections;
    {
        M_LOCK(mutex);
        nOpenedConnections=container_MX.size();
        {
            while (1)
            {
                if (container_MX.size())
                {
                    REF_getter<DBH> dbh=*container_MX.begin();
                    container_MX.pop_front();
                    DBH_postgres * dsql=(DBH_postgres*)dbh.operator ->();

                    if (time(NULL)-dsql->getLastQueryTime()>INACTIVE_CONNECTION_DROP_TIMEOUT_SEC)
                    {
                        continue;
                    }
                    return dbh;
                }
                else break;
            }
        }
    }
    if (nOpenedConnections<20)
    {
        DBH_postgres *dbh=new DBH_postgres(cfg);
        dbh->makeConnection();
        return dbh;
    }
    else
    {
        logErr2("database maximum connection exceeded [%d]",max_connections);
        throw CommonError("database maximum connection exceeded [%d]",max_connections);
    }

}
void DBH_source_postgres::unget(const REF_getter<DBH>& dbh)
{

    if (dbh->bIsValid())
    {
        {
            M_LOCK(mutex);
            container_MX.push_back(dbh);
        }
    }
    else
    {
    }

}
time_t DBH_postgres::getLastQueryTime()
{
    return m_lastQueryTime;
}

void registerPostgresModule(const char*pn)
{
    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Postgres,"postgres");
    }
    else
    {
        iUtils->registerService(ServiceEnum::Postgres,DBH_source_postgres::construct,"postgres");
    }
    XPASS;

}


UnknownBase *DBH_source_postgres::construct(const SERVICE_id& id, const std::string&  nm,IConfigObj* obj)
{
    return new DBH_source_postgres(id,nm,obj);
}

