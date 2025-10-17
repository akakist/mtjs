#include "DBH.h"
#include "IUtils.h"
#include "commonError.h"
#include <deque>

#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
#endif
#ifdef _WIN32
#include <time.h>
#endif
#include "mutexInspector.h"

st_DBH::st_DBH(DBH_source* src, bool _isTransaction): m_source(src),isTransaction(_isTransaction),transactionResult(true),dbh(m_source->get())
{

    MUTEX_INSPECTOR;
    if(isTransaction)
        dbh->exec((std::string)"BEGIN");

}
st_DBH::~st_DBH()
{

    MUTEX_INSPECTOR;
    if(isTransaction)
    {
        if(transactionResult)
            dbh->exec((std::string)"COMMIT");
        else
            dbh->exec((std::string)"ROLLBACK");
    }

    if (!m_source) logErr2("undefined DBH_source");
    m_source->unget(dbh);
}

DBH_source::DBH_source()
{
}

void DBH::insertStrVal(std::map<std::string,std::string>&m,const std::string& name,const char* prefix, const std::string& val, const char* postfix)
{
    MUTEX_INSPECTOR;
    m[name].append(prefix).append(val).append(postfix);
}
std::string DBH::insertString(const std::map<std::string,std::string>&m, const char* tblname)
{
    MUTEX_INSPECTOR;
    std::string ret;
    std::vector<std::string> v1,v2;
    v1.reserve(m.size());
    v2.reserve(m.size());
    for(auto& i:m)
    {
        v1.push_back(i.first);
        v2.push_back(i.second);
    }
    ret.append("insert into ").append(tblname).append("(").append(iUtils->join(",",v1)).append(") values (").append(iUtils->join(",",v2)).append(")");
    return ret;
}


std::string DBH::select_1(const QUERY &q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()!=1) return "";
    if(r->values[0].size()!=1) throw CommonError("if(r->values[0].size()!=1)");
    if(r->values[0].size()==1)
    {
        return r->values[0][0];
    }
    return "";
}
std::string DBH::select_1_orThrow(const QUERY &q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()!=1) throw CommonError("select_1_orThrow: "+q.prepare()+_DMI());
    if(r->values[0].size()!=1) throw CommonError("if(r->values[0].size()!=1)");
    if(r->values[0].size()==1)
    {
        return r->values[0][0];
    }
    return "";
}
std::string DBH::select_1_orThrow(const std::string &q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()!=1) throw CommonError("select_1_orThrow: "+q+_DMI());
    if(r->values.size()==1)
    {
        if(r->values[0].size()!=1) throw CommonError("if(r->values[0].size()!=1)");
        if(r->values[0].size()==1)
        {
            return r->values[0][0];
        }
    }
    std::string rr;
    return rr;
}
std::string DBH::select_1(const std::string &q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()!=1) return "";
    if(r->values.size()==1)
    {
        if(r->values[0].size()!=1) throw CommonError("if(r->values[0].size()!=1)");
        if(r->values[0].size()==1)
        {
            return r->values[0][0];
        }
    }
    std::string rr;
    return rr;
}
int64_t DBH::select_1_int(const QUERY &q)
{

    return atoll(select_1(q).c_str());
}
int64_t DBH::select_1_int_orThrow(const QUERY &q)
{

    return atoll(select_1_orThrow(q).c_str());
}
int64_t DBH::select_1_real_orThrow(const QUERY &q)
{

    return atof(select_1_orThrow(q).c_str());
}


std::vector<std::string> DBH::select_1_row(const std::string&q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()>1) throw CommonError("if(r->values.size()>1)"+_DMI());
    if(r->values.size())
        return r->values[0];

    // std::vector<std::string> ret;
    return std::vector<std::string>();

}
std::vector<std::string> DBH::select_1_row(const QUERY&q)
{
    MUTEX_INSPECTOR;
    REF_getter<QueryResult> r=exec(q);
    if(r->values.size()>1) throw CommonError("if(r->values.size()>1)"+_DMI());
    if(r->values.size())
        return r->values[0];

    // std::vector<std::string> ret;
    return std::vector<std::string> ();

}

std::vector<std::string> DBH::select_1_column(const QUERY&q)
{
    MUTEX_INSPECTOR;
    std::vector<std::string>  ret;
    REF_getter<QueryResult> r=exec(q);
    ret.reserve(r->values.size());
    for(size_t i=0; i<r->values.size(); i++)
    {
        if(r->values[i].size()!=1) throw CommonError("if(r->values[i].size()!=1) "+q.prepare());
        ret.push_back(r->values[i][0]);
    }
    return ret;

}
std::set<std::string> DBH::select_1_columnSet(const QUERY&q)
{
    MUTEX_INSPECTOR;
    std::set<std::string>  ret;
    REF_getter<QueryResult> r=exec(q);
    for(size_t i=0; i<r->values.size(); i++)
    {
        if(r->values[i].size()!=1) throw CommonError("if(r->values[i].size()!=1) "+q.prepare());
        ret.insert(r->values[i][0]);
    }
    return ret;

}

std::vector<std::string> DBH::select_1_column(const std::string&q)
{
    MUTEX_INSPECTOR;
    std::vector<std::string>  ret;
    REF_getter<QueryResult> r=exec(q);
    ret.reserve(r->values.size());
    for(size_t i=0; i<r->values.size(); i++)
    {
        if(r->values[i].size()!=1) throw CommonError("if(r->values[i].size()!=1) "+q);
        ret.push_back(r->values[i][0]);
    }
    return ret;

}
std::set<std::string> DBH::select_1_columnSet(const std::string&q)
{
    MUTEX_INSPECTOR;
    std::set<std::string>  ret;
    REF_getter<QueryResult> r=exec(q);
    for(size_t i=0; i<r->values.size(); i++)
    {
        if(r->values[i].size()!=1) throw CommonError("if(r->values[i].size()!=1) "+q);
        ret.insert(r->values[i][0]);
    }
    return ret;

}


DBH_feature::DBH_feature(IInstance* ifa)
{
    dbh_src=dynamic_cast<DBH_source*>(ifa->getServiceOrCreate(ServiceEnum::Mysql));
    if(!dbh_src)
        throw CommonError("if(!dbh_src)");
}


st_TRANSACTION::st_TRANSACTION(const REF_getter<DBH> &df)
    :dbh(df),exitState(___ROLLBACK)
{
    dbh->exec((std::string)"BEGIN");

}
st_TRANSACTION::~st_TRANSACTION()
{
    switch (exitState) {
    case ___UNDEF:
        logErr2("exitState UNDEF");
        break;
    case ___COMMIT:
        dbh->exec((std::string)"COMMIT");
        break;
    case ___ROLLBACK:
        dbh->exec((std::string)"ROLLBACK");
        break;
    default:
        logErr2("exitState UNDEF");
        break;
    }
}
