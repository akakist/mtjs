#include "blake2bHasher.h"
#include "commonError.h"
#include "NODE_id.h"
#include "REF.h"
#include "ioBuffer.h"
#include "bcEvent.h"
#include "listenerBase.h"
#include "mutexInspector.h"
#include "nodeService.h"
#include <cstdlib>
#include <cstddef>
#include "init_root.h"
#include "tools_mt.h"
std::pair<std::string,std::string> splice(const std::string& id)
{
    switch(id.size())
    {
        case 0: return {"",""};
        case 1: return {"",id};
        case 2: return {id.substr(0,1),id.substr(1,1)};
        case 3: return {id.substr(0,2),id.substr(2,1)};
        case 4: return {id.substr(0,3),id.substr(3,1)};
        case 32:return {id.substr(0,4),id.substr(4,28)};
        default: throw CommonError("defauil switch(id.size())");
    }
}
void collect_sync_req2(const std::string& id, const std::string &granule,IDatabase* db)
{
    {
        inBuffer in(granule);
        REF_getter< Cellable> c=new Cellable(NULL,id);
        c->unpack_mx(in);
        for(auto& z: c->children_hashes_mx)
        {
            std::string cid=id+z.first;
            std::string cbuf;
            auto r2=db->getGranule(cid, &cbuf);
            auto h=blake2b_hash(cbuf);
            if(h!=z.second)
            {
                db->add_sync_out(cid);
            }
        }
    }
}

bool Node::Service::GetGranulesREQ(const bcEvent::GetGranulesREQ*e)
{

    // bcEvent::GetGranulesRSP()
    std::vector<std::pair<std::string,std::string>> ret;
    std::string rg;
    int err=db_state->getGranule("",& rg);
    if(err)
    {
        logNode("root granule not found");
        return true;
    }
    auto root_hash=blake2b_hash(rg);
    for(auto& z: e->keys)
    {
        err=db_state->getGranule(z,& rg);
        if(err)
        {
            logNode("granule not found %s",base16::encode(z).c_str());
            return true;
        }
        logErr2("GetGranulesREQ: ret add %s",base16::encode(z).c_str());
        ret.push_back({z,rg});
    }
    passEvent(new bcEvent::GetGranulesRSP(root_hash, this_node_name, ret, poppedFrontRoute(e->route)));
    return true;    
}

bool Node::Service::GetGranulesRSP(const bcEvent::GetGranulesRSP* m)
{

    // root->getLeafNoCreate()
    // getByPathNoCreate()

    
    _db_to_save db;

    for(auto& z: m->v)
    {
        if(z.first=="")
        {

        }
        else 
        {
            logNode("GetGranulesRSP: recv granule '%s'",base16::encode(z.first).c_str());
            auto id=splice(z.first);
            std::string pbuf;
            auto r2=db_state->getGranule(id.first, &pbuf);
            // if(r2) throw CommonError("if(r2) ");
            if(pbuf.size())
            {
                REF_getter< Cellable> par=new Cellable(NULL,id.first);
                inBuffer inpar(pbuf);
                par->unpack_mx(inpar);
                auto it=par->children_hashes_mx.find(id.second);
                if(it==par->children_hashes_mx.end())
                    throw CommonError("if(it==par->children_hashes_mx.end())");
                if(it->second!=blake2b_hash(z.second))
                    throw CommonError("if(it->second!=blake2b_hash(z.second))");
            }


        }
        db.add(z.first,z.second);
    }
    db_state->write_granules_batch(db);
    for(auto &z :m->v)
    {
        db_state->remove_sync_out(z.first);
    }
    for(auto &z :m->v)
    {
        collect_sync_req2(z.first,z.second, db_state.get());
    }
    std::vector<std::string> pathes=db_state->getPathes();
    if(pathes.size())
    {
        auto n=db_state->getNode(m->responder);
        auto ip=n->get_ip();
        sendEvent(ip,ServiceEnum::Node,new bcEvent::GetGranulesREQ(pathes,ListenerBase::serviceId));

    }
    else{
    }
    return true;
}
void Node::Service::continue_sync()
{
    // std::vector<std::string> pathes;
    auto p=db_state->getPathes();
    sendEvent(db_state->getIp(),ServiceEnum::Node,new bcEvent::GetGranulesREQ(p,ListenerBase::serviceId));

}

void Node::Service::do_sync(const NODE_id &src_node, const THASH_id& prev_root_hash_remote)
{
    MUTEX_INSPECTOR;


    logNode ("do_sync");

    // std::vector<std::string> pathes;
    // pathes.push_back("");
    db_state->add_sync_out("");

    auto n=db_state->getNode(src_node);
    // root=NULL;
    auto ip=n->get_ip();
    db_state->setIp(ip);
    continue_sync();

    // sendEvent(db_state->getIp(),ServiceEnum::Node,new bcEvent::GetGranulesREQ(pathes,ListenerBase::serviceId));


}
bool Node::Service::DelayNotificationREQ(const MsgData::DelayNotificationREQ *r, const NODE_id &src_node, const route_t &route)
{
    bool remote_verified=verify_block(r->lc);
    if(!remote_verified)
        return true;
    // auto local_lc=prev_block;
    if(!prev_block.valid() || r->lc->blockInfo->heart_beat->new_epoch > prev_block->blockInfo->heart_beat->new_epoch)
    {
        MUTEX_INSPECTOR;
        // state_Z=STATE_SYNCING;
        logNode("STATE_SYNCING");
        prev_block=r->lc;
        do_sync(src_node,r->lc->blockInfo->new_root_hash1);
        // r->lc->blockInfo->heart_beat->prev_root_hash_1;
    }


    return true;
}
