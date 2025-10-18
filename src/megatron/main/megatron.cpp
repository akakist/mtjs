#define _FILE_OFFSET_BITS 64
#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <stdio.h>
#ifdef _WIN32
#else
#include <dlfcn.h>
#endif
#include "megatron_config.h"
#include "megatron.h"
#include "CUtils.h"
#include "configObj.h"
#include "commonError.h"
#ifndef _MSC_VER
#endif


//IInstance* iInstance=NULL;
//
#ifndef _WIN32
//std::string gCacheDirNoCheck();
#endif

#if !defined __MOBILE__

void registerHTTPModule(const char*);
void registerSqliteModule(const char*);
void registerTelnetService(const char*);
#ifdef WEBDUMP
void registerWebHandlerModule(const char*);
#endif
void registerErrorDispatcherService(const char*);
#endif

void registerObjectProxyModule(const char*);
void registerOscarModule(const char*);
// void registerOscarSecureModule(const char*);
void registerRPCService(const char*);
void registerSocketModule(const char*);
void registerTimerService(const char*);
void registerReferrerClientService(const char* pn);
static void registerModules()
{
#ifdef __FULL__
    char *pn=NULL;
    {
#if !defined __MOBILE__
        registerHTTPModule(pn);
        registerTelnetService(pn);
        registerWebHandlerModule(pn);
        registerErrorDispatcherService(pn);
#endif
        registerObjectProxyModule(pn);
        registerOscarModule(pn);
        // registerOscarSecureModule(pn);
        registerRPCService(pn);
        registerSocketModule(pn);
        registerTimerService(pn);
        registerReferrerClientService(pn);
    }
    void registerSSL(const char* pn);
    registerSSL(pn);
#endif
}
#ifdef _WIN32
static std::vector<std::string> splitString(const char *seps, const std::string & src)
{


    std::vector < std::string> res;
    std::set<char>mm;
    size_t l;
    l =::strlen(seps);
    for (unsigned int i = 0; i < l; i++)
    {
        mm.insert(seps[i]);
    }
    std::string tmp;
    l = src.size();

    for (unsigned int i = 0; i < l; i++)
    {

        if (!mm.count(src[i]))
            tmp += src[i];
        else
        {
            if (tmp.size())
            {
                res.push_back(tmp);
                tmp.clear();
            }
        }
    }

    if (tmp.size())
    {
        res.push_back(tmp);
        tmp.clear();
    }
    return res;
}
#endif
#if !defined __MOBILE__

#ifdef __MAKE_PID
bool megatron::findProcess(const char* _process)
{


    std::string pr=_process;
#if defined(_WIN32) && defined(QT5)
    Sleep(100);
    std::string::size_type pz=pr.rfind("\\");
    if(pz==std::string::npos)
    {
        pz=pr.rfind("/");
    }


    if(pz!=std::string::npos)
    {
        pr=pr.substr(pz+1,pr.size()-pz-1);
    }
    QProcess tasklist;
    tasklist.start(
        "tasklist",
        QStringList() << "/NH");
    tasklist.waitForFinished();
    QString output = tasklist.readAllStandardOutput();
    std::vector<std::string> v=splitString("\n\r",output.toStdStringView());
    std::map<std::string,std::set<size_t> > procs;
    for(size_t i=0; i<v.size(); i++)
    {
        std::vector<std::string> vv=splitString(" ",v[i]);
        if(vv.size())
        {

            procs[vv[0]].insert(i);
        }
    }
    return procs[pr].size()>1;

#endif

#ifndef _WIN32
    std::string proc=gCacheDirNoCheck() + "/" +_process +".pid";
    FILE* f=fopen(proc.c_str(),"r");

    int n=0;
    if(f)
    {

        int r=fscanf(f,"%d",&n);
        fclose(f);
        if(r==-1)
        {

            char *s=strerror(errno);
            //logErr2("fscanf %s",s);
        }

        if(r==1)
        {
            int rk=kill(n,SIGHUP);
            if(rk)
            {

                return false;
            }
            else
            {

                return true;
            }
        }

    }
    return false;
#endif
}
#endif
#endif

#ifdef _WIN32
static bool win32Initialized=false;
#endif
void megatron::run(
    int argc, char* argv[]
)
{

    try {

#ifdef _WIN32
        if(!win32Initialized)
        {
            win32Initialized=true;
            WORD wVersionRequested;
            WSADATA wsaData;
            int err;
            wVersionRequested = MAKEWORD(2, 2);
            err = WSAStartup(wVersionRequested, &wsaData);
            if (err != 0)
            {
                logErr2("WSAStartup failed\n");
                return ;
            }
        }
#endif



        std::string app_name;

        if(argc>0) {
            /// calc app name;

            std::string f=argv[0];
            std::string::size_type  zz;
#ifdef _WIN32
            zz = f.rfind("\\", f.size());
#else
            zz = f.rfind("/", f.size());
#endif
            if (zz != std::string::npos)
            {
                app_name=f.substr(zz + 1, f.size() - zz - 1);
            }
            zz = app_name.rfind(".", app_name.size());
            if (zz != std::string::npos)
            {
                app_name=app_name.substr(0,zz);
            }
        }

        if(argc==2 && std::string(argv[1])=="--help")
        {

            printf( "Usage:\n"
                    "\t-d - run as daemon\n"
                    "\t--help - print this page\n"
                  );
            exit(0);
        }

        iUtils=new CUtils(argc,argv,app_name);
        //runned=true;
        iInstance=iUtils->createNewInstance(app_name);

        {
#if !defined __MOBILE__

#ifndef _WIN32
#ifdef __MAKE_PID
            std::string pidfname=gCacheDir()+ "/" +app_name +".pid";
            FILE* f=fopen(pidfname.c_str(),"w");
            if(!f)
            {
                fprintf(stderr,"cannot open %s\n",pidfname.c_str());
                exit(1);
            }
            fprintf(f,"%d\n",getpid());
            fclose(f);
#endif
#endif
#endif
        }


        ConfigObj *cnf=NULL;


#ifndef _WIN32
        {
#if defined __MOBILE__
            cnf=new ConfigObj;
#else
            std::string pn=iUtils->gConfigDir()+ "/"+app_name+".conf";
            logErr2("pn %s",pn.c_str());
            cnf=new ConfigObj(pn);
#endif

        }
#else
        cnf=new ConfigObj(app_name+".conf");
#endif


        config=cnf;
        iInstance->setConfig(cnf);
#ifdef __MOBILE__
        //factory->m_no_bind=cnf->get_bool("no_bind",false,"Do not listen any port");
#else
        //factory->m_no_bind=cnf->get_bool("no_bind",false,"Do not listen any port");
#endif

        std::set<std::string> sp;

        sp=config->get_stringset("PluginDir",PLUGIN_TARGET_DIR,"Search paths of dfs plugins");
        iUtils->load_plugins_info(sp);

        registerModules();
        iInstance->initServices();
    }
    catch(const std::exception &e)
    {
        printf("catched %s",e.what());
        exit(0);

    }





}


megatron::~megatron()
{
#ifdef __MAKE_PID
    std::string fn=iInstance->app_name();
    std::string pidfname=giUtils->CacheDir()+ "/" +fn +".pid";
    unlink(pidfname.c_str());
#endif

    if(iUtils)
        delete iUtils;
}
megatron::megatron()
    :config(NULL)
{
}

