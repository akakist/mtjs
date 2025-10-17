#pragma once
#include "SERVICE_id.h"
#include "_QUERY.h"
#include "queryResult.h"
#include "unknown.h"
#include "IInstance.h"
#include "REF.h"

#include "DBHEvent.h"
#define SQL_BUFFER_SIZE 4096
/**  Universal device independant database handler
*/

class DBH: public Refcountable
{
public:
    enum engine {mysql,sqlite,postgres};

    virtual void execSimple(const QUERY &query)=0;

    ///   Execute SQL and return QueryResult.
    virtual REF_getter<QueryResult> exec(const QUERY &)=0;

    virtual void execSimple(const std::string &query)=0;

    ///   Execute SQL and return QueryResult.
    virtual REF_getter<QueryResult> exec(const std::string &)=0;

    ///   Escape chars using db engine
    virtual std::string escape(const std::string&)=0;

    virtual time_t getLastQueryTime()=0;

    virtual ~DBH() {}
    virtual bool bIsValid()=0;

    /// select only 1 value - int, string, with exception or not
    std::string select_1(const QUERY &);
    int64_t select_1_int(const QUERY &);
    std::string select_1_orThrow(const QUERY &);
    int64_t select_1_int_orThrow(const QUERY &);
    int64_t select_1_real_orThrow(const QUERY &);


    /// get single row
    std::vector<std::string> select_1_row(const QUERY&);
    std::vector<std::string> select_1_row(const std::string &);

    /// get single col
    std::vector<std::string> select_1_column(const QUERY&);
    std::set<std::string> select_1_columnSet(const QUERY&q);


    /// get 1 element
    std::string select_1(const std::string &);
    std::string select_1_orThrow(const std::string &);

    /// get 1 column
    std::vector<std::string> select_1_column(const std::string &);
    std::set<std::string> select_1_columnSet(const std::string&q);

    static void insertStrVal(std::map<std::string,std::string>&m,const std::string& name,const char* prefix, const std::string& val, const char* postfix);

    static std::string insertString(const std::map<std::string,std::string>&m, const char* tblname);

protected:
};


/// db source controller
class DBH_source
{
public:
    virtual REF_getter<DBH> get()=0;
    virtual void unget(const REF_getter<DBH>&)=0;
    DBH_source();
    virtual ~DBH_source() {}
};

/// stacked holder of db connection. On constructor extracts it from heap, on destructor - puts back
class st_DBH
{
private:
    DBH_source* m_source;
    bool isTransaction;
    bool transactionResult;
public:
    REF_getter<DBH> dbh;

    st_DBH(DBH_source* src, bool transaction);
    ~st_DBH();
};


/// base class to allow inherited service to use DB connection
struct DBH_feature
{
private:
    DBH_source *dbh_src;
    Mutex mx;
    std::map<pthread_t,std::pair<REF_getter<DBH>,time_t > > m_sources;
public:
    DBH_feature(IInstance* ifa);
    REF_getter<DBH> getDB()
    {
        pthread_t pt=pthread_self();
        M_LOCK(mx);
        auto it=m_sources.find(pt);
        if(it!=m_sources.end())
        {
            if(time(NULL)-it->second.second>600)
            {
                m_sources.erase(pt);
            }
            else
            {
                return m_sources.find(pt)->second.first;
            }
        }

        REF_getter<DBH> d = dbh_src->get();
        m_sources.insert(std::make_pair(pt,std::make_pair(d,time(NULL))));
        return d;
    }
};

/// stacked transaction holder. On constructor transaction begins. On destructor by default makes "rollback".
/// If need to commit before destructor - use 'commit' method
struct st_TRANSACTION
{
    REF_getter<DBH> dbh;
    enum
    {
        ___UNDEF,___COMMIT,___ROLLBACK
    };
    int exitState;
    void commit()
    {
        exitState=___COMMIT;
    }
    void rollback()
    {
        exitState=___ROLLBACK;
    }
    st_TRANSACTION(const REF_getter<DBH> &df);
    ~st_TRANSACTION();

};
