#include "IInstance.h"
#include "configObj.h"
#include "CUtils.h"
#include <unistd.h>
#include "commonError.h"
#include <iostream>
#include "common/mtjsEvent.h"
void registerRPCService(const char* pn);
#ifdef __linux__
#endif
void registerMTJSModule(const char* pn);
void registerSocketModule(const char* pn);
void registerTimerService(const char* pn);
void registerOscarModule(const char* pn);
void registerHTTPModule(const char* pn);
void registerMysqlModule(const char*pn);
void registerWebHandlerModule(const char* pn);
void registerTelnetService(const char*);


struct par {
    int n; /// n defises
    bool hasParam;
    std::string name;
    std::string default_v;
    std::string *varString;
    bool *varBool;
    const char* descr;
};

void help(const std::vector<par>& v)
{


    // std::cout<< h;
    std::cout<< "mtjs - JavaScript Engine\n\n";
    std::cout<< "Usage:\n"
             "\tmtjs [options] [script.js]\n";

    std::cout<< "Options:\n\n";
    for(auto &z: v)
    {
        for(int i=0; i<z.n; i++)
            std::cout<<"-";
        if(z.descr) {
            std::cout << z.name  << z.descr;
            std::cout<< "\n";
        }
    }
    exit(1);
}

int mainMTJS(int argc, char** argv )
{
    // curl_global_init(CURL_GLOBAL_DEFAULT);

    try {
        iUtils=new CUtils(argc, argv, "mtjs");


        registerOscarModule(NULL);
        registerHTTPModule(NULL);
        registerSocketModule(NULL);
        registerTimerService(NULL);
        registerRPCService(NULL);
        registerTelnetService(NULL);

        registerMTJSModule(NULL);

#ifdef WEBDUMP
        registerWebHandlerModule(NULL);
#endif


#ifdef __linux__
        registerMysqlModule(NULL);
#endif

        std::string mtjs_STACK_SIZE="8388608";
        std::string mtjs_pool_size="10";
        std::string js;
        std::string SocketIO_n_workers="10";
        std::string SocketIO_epoll_timeout_millisec="2000";
        std::string mtjs_pending_timeout="0.200000";


        bool isDaemon=false;
        bool isHelp=false;
        bool isVersion=false;
        std::vector<par> v =
        {
            {   1,true,"e","",& js,nullptr,R"(, --eval <code>
    Execute JavaScript code passed as <code> from the command line.
    Useful for quick tests or inline scripts.
)"},
{1,false,"d","",nullptr,&isDaemon,R"(, --daemon
    Run mtjs as a daemon (in the background). The process will detach from the console.
)"},
{2,false,"daemon","",nullptr,&isDaemon,nullptr},
{1,false,"h","",nullptr,&isHelp,R"(, --help
      Display this help message and exit.
)"},        
{2,false,"help","",nullptr,&isHelp,nullptr},
{1,false,"v","",nullptr,&isVersion,R"(, --version
    Print mtjs version information and exit.
)"},
{2,false,"version","",nullptr,&isVersion,nullptr},

          };

          // EAGAIN
        std::vector<std::string> unpaired;
        std::deque<std::string> dq;
        for(int i=1;i<argc;i++)
        {
            dq.push_back(argv[i]);
        }

        while(dq.size())
        {
            auto s=dq[0];
            dq.pop_front();
            bool used=false;
            for(auto & z: v)
            {
                std::string defises;
                for(int j=0;j<z.n;j++)
                    defises+="-";
                
                if(defises+z.name==s)
                {
                    used=true;
                    if(z.hasParam)
                    {

                        if(!dq.size())
                        {
                            help(v);
                        }
                        auto p=dq[0];
                        dq.pop_front();
                        if(z.varString)
                        {
                            *z.varString=p;
                        }
                        else
                        {
                            help(v);
                        }
                    }
                    else
                    {
                        if(z.varBool)    
                        *z.varBool=true;
                    }
                }
            }
            if(!used)
                unpaired.push_back(s);
            

        }
        if(isVersion)
        {
          #define mtjs_VERSION "0.1.0"
            std::cout<< "mtjs - JavaScript Engine\n";
            std::cout<< "mtjs version: "<<mtjs_VERSION<<std::endl;
            std::cout<< "mtjs build date: "<<__DATE__<<std::endl;
            std::cout<< "mtjs build time: "<<__TIME__<<std::endl;
            exit(0);
        }
        if(isHelp)
        {
            help(v);
        }
        if(unpaired.size()>1)
        {
          std::cout<< "To many scripts\n";
          exit(1);
            // help(v);
        }
        if(unpaired.size() && !js.empty())
        {
          std::cout<< "To many scripts\n";
          exit(1);
        }
        if(unpaired.size()==1 && js.empty())
            js=iUtils->load_file(unpaired[0]);

        if(js.empty())
        {
            std::cout<< "No script specified\n";
            help(v);
            exit(1);
          }
        if(isDaemon)
        {
          if (fork()) exit(1);
        }

        {
            IInstance *instance1=iUtils->createNewInstance("MTJS");
            ConfigObj *cnf1=new ConfigObj("MTJS",
        				    (std::string)
                                          "\nStart=MTJS"+
                                          #ifdef __linux__
//                                          ",DataFeedMWReader"+
                                          #endif
                                          "\nMTJS_STACK_SIZE="+mtjs_STACK_SIZE+
                                          "\nMTJS_THREAD_POOL_SIZE="+mtjs_pool_size+
                                          "\nmtjs_deviceName=Device"
                                          "\nSocketIO_listen_backlog=128"
                                          "\nSocketIO_size=1024"
                                          "\nSocketIO_epoll_timeout_millisec="+SocketIO_epoll_timeout_millisec+
                                          "\nSocketIO_n_workers="+SocketIO_n_workers+
                                          "\nOscar_maxPacketSize=33554432"
                                          "\nMTJS_SERVER_NAME=WebServer"
                                          "\nMTJS_JS_FN="+std::string(argv[1])+""
#ifdef WEBDUMP                                        
                                          "\nWebHandler_bindAddr=INADDR_ANY:5555"
#endif
                                          "\nHTTP_max_post=1000000"
                                          "\nHTTP_doc_urls=/pics,/html,/css"
                                          "\nHTTP_document_root=./www"
                                          "\nMTJS_PENDING_TIMEOUT="+mtjs_pending_timeout+
					    ""
                                          
                                         );
            instance1->setConfig(cnf1);
            instance1->initServices();

            instance1->sendEvent(ServiceEnum::mtjs,new mtjsEvent::Eval(js,{}));
        }

        

        sleep(2);
//        system("mtjs 127.0.0.1 8081");
        while(!iUtils->isTerminating())
            sleep(1);
        delete iUtils;
        return 0;

    } catch (CommonError& e)
    {
        printf("CommonError %s\n",e.what());
    }
    // curl_global_cleanup();

    return 1;
}
