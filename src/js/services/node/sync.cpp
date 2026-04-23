#include "nodeService.h"
#include <SQLiteCpp/Database.h>
#include "tools_mt.h"
void Node::Service::on_get_blocks_req(const msg::get_blocks_req &r, const route_t& route)
{
    SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READONLY);

    SQLite::Statement query(dbs, "SELECT epoch, data FROM blocks WHERE epoch>=? order by epoch limit 100");
    query.bind(1,r.myEpoch.toString());
    msg::get_blocks_rsp ret;
    while (query.executeStep())
    {
        // BigInt ep;
        BigInt ep = query.getColumn(0).getInt();
        std::string data = query.getColumn(1).getString();
        ret.blocks_Z.push_back({ep,iUtils->hex2bin(data)});
    }
    ret.lastEpoch=root->getValues(NULL)->epoch;
    msg::node_message_ed nm(ret.getBuffer(),this_node_name,my_sk_ed);
    passEvent(new bcEvent::MsgReply(nm.getBuffer(),poppedFrontRoute(route)));

}

void Node::Service::do_sync()
{
    // last_leader_cert
    if(last_leader_cert.empty())throw CommonError("if(last_leader_cert.empty())");
    msg::leader_certificate lc(last_leader_cert);
    // lc.nodes
    std::vector<NODE_id> node_names;
    for(auto& z: lc.nodes)
    {
        if(z!=this_node_name)
            node_names.push_back(z);
    }
    auto nn=node_names[rand()%lc.nodes.size()];
    logNode("selected node %s",nn.container.c_str());
    auto n=root->getNode(nn,NULL);
    if(!n.valid())
        throw CommonError("if(!n.valid())");
    msg::get_blocks_req gbr;
    gbr.myEpoch=root->getValues(NULL)->epoch;
    msg::node_message_ed nm(gbr.getBuffer(),this_node_name,my_sk_ed );
    sendEvent(n->ip,ServiceEnum::Node, new bcEvent::Msg(nm.getBuffer(),ListenerBase::serviceId));

}
void Node::Service::on_get_blocks_rsp(const msg::get_blocks_rsp& r)
{
    // logNode("on_get_blocks_rsp");
    for(auto& z: r.blocks_Z)
    {
        // auto & epoch=z.first
        msg::publish_block pb(z.second);
        msg::block_accepted_req ba(pb.block_accepted_req);
        msg::leader_certificate lc(ba.leader_certificateZ);
        msg::blockZ bl(ba.block_payload);
        msg::heart_beat hb(bl.payload_heart_bit);
        if(hb.epoch!=z.first)
            throw CommonError("if(hb.epoch!=z.first)");
        if(hb.epoch!=root->getValues(NULL)->epoch)
            throw CommonError("if(hb.epoch!=root->getValues(NULL)->epoch) %s %s",hb.epoch.toString().c_str(), root->getValues(NULL)->epoch.toString().c_str()   );

        bls::PublicKey agg_pk;
        agg_pk.clear();
        for(auto& k: ba.node_validators)
        {
            auto n=root->getNode(k,NULL);
            agg_pk.add(n->bls_pk);
        }
        if(!ba.agg_sig.verify(agg_pk,blake2b_hash(ba.block_payload).container))
        {
            throw CommonError("on_get_blocks_rsp: !ba.agg_sig.verify");
        }
        logNode("on_get_blocks_rsp: block verified OK");
        // logNode("on_get_blocks_rsp: my epoch %s hb.epoch %s",root->getValues(NULL)->epoch.toString().c_str(), hb.epoch.toString().c_str() );

        SQLite::Database dbs(sqlite_pn, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        dbs.exec("CREATE TABLE IF NOT EXISTS blocks ("
                 "epoch INTEGER PRIMARY KEY, "
                 "data BLOB NOT NULL)");

        SQLite::Statement insert(dbs, "INSERT INTO blocks (epoch, data) VALUES (?, ?)");
        insert.bind(1,pb.epoch.toString());
        insert.bind(2,iUtils->bin2hex(pb.getBuffer()));
        insert.exec();

        auto new_root_hash=execute_block(root, prev_block_hash, pb.att_data.trs,lc.nodes);
        if(new_root_hash==bl.new_root_hash1)
        {
            logNode("on_get_blocks_rsp: block executed OK on epoch %s",hb.epoch.toString().c_str());
            root->getValues(NULL)->epoch=hb.epoch+1;
            db->write_batch(db_to_save_Z);
            db_to_save_Z.clear();

            prev_block_hash=new_root_hash;

        }
        else
        {
            throw CommonError("if(new_root_hash!=bl.new_root_hash1)");
        }




    }
    if(r.lastEpoch > root->getValues(NULL)->epoch)
    {
        logNode("do_sync again: r.lastEpoch %s > root->getValues(NULL)->epoch %s",r.lastEpoch.toString().c_str(), root->getValues(NULL)->epoch.toString().c_str() );
        do_sync();
        return;
    }
    state_Z=State::NORMAL;
    logNode("State::NORMAL");
}
