#pragma once
#include "REF.h"
#include "epoll_socket_info.h"

namespace referrerClient {
    struct _uplinkConnectionState: public Refcountable
    {

        _uplinkConnectionState(const std::set<msockaddr_in> & _saCaps):
//            getServiceSent(false),
            saCapsFromConfig(_saCaps),
            connectionEstablished(false),
            nodeLevelInHierarhy(0)

        {}
        _uplinkConnectionState():
//            getServiceSent(false),
            connectionEstablished(false),
            nodeLevelInHierarhy(0)
        {

        }






//        bool getServiceSent;
        std::set<msockaddr_in> saCapsFromConfig;


        bool connectionEstablished;
        std::set<msockaddr_in> uplink;
        int nodeLevelInHierarhy;
        std::set<REF_getter<epoll_socket_info> >ponged_for_cleanup_sockets;
        std::map<int64_t/*ping time*/,std::set<std::pair<REF_getter<epoll_socket_info>,msockaddr_in > > > ponged_all;



    };

}
