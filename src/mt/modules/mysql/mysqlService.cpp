#include <stdio.h>
#include "DBH.h"
#include "IUtils.h"
#include "mysqlService.h"
#include <string>
#include <mariadb/mysql.h>
#include "commonError.h"
#ifdef _WIN32
#include "compat_win32.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#endif
#include <deque>
#include <stdarg.h>
#include "IConfigObj.h"
#include "IInstance.h"
#include "DBH.h"
#ifndef _WIN32
#include <alloca.h>
#endif
#ifdef _WIN32
#pragma comment(lib, "libmysql.lib")
#endif
#include "_QUERY.h"
#include "st_malloc.h"
#include "mutexInspector.h"

bool DBH_mysql::bIsValid()
{

    if (!dbh) return false;
    if (bIsLost) return false;
    return true;
}

void DBH_mysql::execSimple(const QUERY &q)
{
    exec(q);
}
void DBH_mysql::execSimple(const std::string &q)
{
    exec(q);
}


REF_getter<QueryResult> DBH_mysql::exec(const QUERY &query)
{

    XTRY;
    std::string result;
    //bind.
    size_t qidx=0;
    for (unsigned int i=0; i<query.query_.size(); i++)
    {
        if (query.query_[i]!='?') result+=query.query_[i];
        else
        {
            if (query.params_.size()<=qidx)throw CommonError("query params size not equal ? count"+_DMI());
            result+=escape(query.params_[qidx]);
            qidx++;
        }
    }
    return exec(result);

    XPASS;

}

REF_getter<QueryResult> DBH_mysql::exec(const std::string &query)
{
//    logErr2("result %s",result.c_str());
    REF_getter<QueryResult> rr(new QueryResult);
    XTRY;
    m_lastQueryTime=time(NULL);
    int status=mysql_real_query(dbh, query.data(), (unsigned long)query.size());
    if (status)  	//query error
    {
        bIsLost=true;
        logErr2("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
        throw CommonError("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
    }


    int sets_cnt=0;

    do
    {
        MYSQL_RES *mres=mysql_store_result(dbh);
        if (mres)
        {
            try
            {
                int num_fields = mysql_num_fields(mres);
                while (1)
                {
                    MYSQL_ROW row=mysql_fetch_row(mres);

                    if (!row) break;

                    unsigned long *lengths = mysql_fetch_lengths(mres);
                    std::vector <std::string> v;
                    for (int i = 0; i < num_fields; i++)
                    {
                        std::string s = "";
                        if (row[i])
                        {
                            std::string a(row[i], lengths[i]);
                            s = a;
//              		    logErr2("a: %s",a.c_str());
                        }
                        v.push_back(s);
                    }
                    rr->values.push_back(v);
                }
            }
            catch (...)
            {
                mysql_free_result(mres);
                bIsLost=true;
                throw;
            }

            mysql_free_result(mres);
            sets_cnt++;
        }
        else
        {
            if (mysql_field_count(dbh) == 0)
            {
            }
            else
            {
                bIsLost=true;
                logErr2("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
                throw CommonError("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
            }
        }
        if ((status = mysql_next_result(dbh)) > 0)
        {
            bIsLost=true;
            logErr2("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
            throw CommonError("exec: mysql_real_query failed on query: [%s] error [%s]", query.c_str(),mysql_error(dbh));
        }
    }
    while (status == 0);

    if (sets_cnt>1)
    {
        logErr2("exec failed: multiresults returned for query [%s], use exec3 for this.", query.c_str());
        throw CommonError("exec failed: multiresults returned for query [%s], use exec3 for this.", query.c_str());
    }
    XPASS;
    return rr;
}

std::string DBH_mysql::escape(const std::string& s)
{

    //char* to=(char*)alloca(s.size()*2+2);
    st_malloc buf(s.size()*2);
//    if (!buf.buf) throw CommonError("alloca failed");
    size_t len=mysql_real_escape_string(dbh, (char*)buf.buf, s.data(), (unsigned long)s.size()) ;
    return std::string((char*)buf.buf,len);

}
void DBH_mysql::makeConnection(const mysqlConf& conf)
{
    dbh=new MYSQL;
    mysql_init(dbh);
    // my_bool enforce_tls= 0;
    // mysql_optionsv(dbh, MYSQL_OPT_SSL_ENFORCE, (void *)&enforce_tls);

    // dbh->options.use_ssl=0;
    // dbh->options.s

    MYSQL* r=mysql_real_connect(dbh,
                                conf.host=="NULL"?NULL:conf.host.c_str(),
                                conf.user=="NULL"?NULL:conf.user.c_str(),
                                conf.passwd=="NULL"?NULL:conf.passwd.c_str(),
                                conf.database=="NULL"?NULL:conf.database.c_str(),
                                conf.port,
                                conf.sock=="NULL"?NULL:conf.sock.c_str(),
                                conf.flag);
    if (!r)
    {

        const char *e=mysql_error(dbh);
        char errs[500];
        snprintf(errs,sizeof(errs),"mysql_real_connect to '%s' failed [%s] in svs",conf.database.c_str(),e?e:"mysql error undefined");
        logErr2(errs);
        delete dbh;
        dbh=NULL;
        logErr2(errs);
        throw CommonError(errs);
    }
    // my_bool enforce_tls= 0;
    // mysql_optionsv(dbh, MYSQL_OPT_SSL_ENFORCE, (void *)&enforce_tls);

    // if (conf.start_code.size())
    // {
    //     execSimple(conf.start_code);
    // }
}
DBH_mysql::~DBH_mysql()
{

    if (dbh)
    {
        //  logErr2("mysql_close(dbh);");
        mysql_close(dbh);
        delete dbh;
        dbh=NULL;
    }
}



DBH_mysql::DBH_mysql()
    :DBH(),dbh(NULL),bIsLost(false),m_lastQueryTime(time(NULL))
{
}
DBH_source_mysql::DBH_source_mysql(const SERVICE_id& _id, const std::string&  nm,IInstance* ifa)
    : UnknownBase(nm),
      ListenerSimple(nm,_id),
      max_connections(10)
{

    IConfigObj* conf=ifa->getConfig();
    cfg.max_connections=conf->get_int64_t2("mysql_max_connections",20,"");
    cfg.host=conf->get_string2("mysql_host","NULL","");
    cfg.user=conf->get_string2("mysql_user","root","");
    cfg.passwd=conf->get_string2("mysql_passwd","NULL","");
    cfg.database=conf->get_string2("mysql_database","md","");
    // cfg.start_code=conf->get_string2("mysql_start_code","SET NAMES utf8","");
    cfg.port=conf->get_int64_t2("mysql_port",0,"");
    cfg.sock=conf->get_string2("mysql_sock","NULL","");
    std::string sflag=conf->get_string2("mysql_flag","","");
    std::vector<std::string> v=iUtils->splitString("|",sflag);
    cfg.flag=0;
    for (size_t i=0; i<v.size(); i++)
    {
        if (v[i]=="CLIENT_COMPRESS") cfg.flag|=CLIENT_COMPRESS;
        else if (v[i]=="CLIENT_FOUND_ROWS") cfg.flag|=CLIENT_FOUND_ROWS;
        else if (v[i]=="CLIENT_IGNORE_SPACE") cfg.flag|=CLIENT_IGNORE_SPACE;
        else if (v[i]=="CLIENT_INTERACTIVE") cfg.flag|=CLIENT_INTERACTIVE;
        else if (v[i]=="CLIENT_LOCAL_FILES") cfg.flag|=CLIENT_LOCAL_FILES;
        else if (v[i]=="CLIENT_MULTI_STATEMENTS") cfg.flag|=CLIENT_MULTI_STATEMENTS;
        else if (v[i]=="CLIENT_MULTI_RESULTS") cfg.flag|=CLIENT_MULTI_RESULTS;
        else if (v[i]=="CLIENT_NO_SCHEMA") cfg.flag|=CLIENT_NO_SCHEMA;
        else if (v[i]=="CLIENT_ODBC") cfg.flag|=CLIENT_ODBC;
        else if (v[i]=="CLIENT_SSL") cfg.flag|=CLIENT_SSL;
        else
            logErr2("invalid mysql flag %s",v[i].c_str());
    }
}
DBH_source_mysql::~DBH_source_mysql()
{

}


REF_getter<DBH> DBH_source_mysql::get()
{
    //DBH_mysql *dbh=NULL;
    int nOpenedConnections;
    {
        M_LOCK(this);
        nOpenedConnections=container_MX.size();
        {
            while (1)
            {
                if (container_MX.size())
                {
                    REF_getter<DBH> dbh=*container_MX.begin();
                    container_MX.pop_front();

                    DBH_mysql* dsql=(DBH_mysql* )dbh.get();

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
    if (nOpenedConnections<max_connections)
    {
        DBH_mysql*dbh=new DBH_mysql;
        dbh->makeConnection(cfg);
        return dbh;
    }
    else
    {
        throw CommonError("database maximum connection exceeded [%d]",max_connections);
    }
}
void DBH_source_mysql::unget(const REF_getter<DBH>& dbh)
{

    if (dbh->bIsValid())
    {
        {
            M_LOCK(this);
            container_MX.push_back(dbh);
        }
    }
    else
    {
    }

}
bool  DBH_source_mysql::handleEvent(const REF_getter<Event::Base>&e)
{
    return true;
}

time_t DBH_mysql::getLastQueryTime()
{
    return m_lastQueryTime;
}
void registerMysqlModule(const char*pn)
{
//    iUtils->registerService(COREVERSION,ServiceEnum::Mysql,DBH_source_mysql::construct,"mysql");
    XTRY;
    if(pn)
    {
        std::set<EVENT_id>evts;
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Mysql,"mysql",evts);
    }
    else
    {
        iUtils->registerService(ServiceEnum::Mysql,DBH_source_mysql::construct,"mysql");
    }
    XPASS;

}

UnknownBase* DBH_source_mysql::construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
{
    return new DBH_source_mysql(id,nm,ifa);

}
