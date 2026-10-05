#pragma once
#include <ioBuffer.h>
#include <blake2bHasher.h>
#include "msgFactory.h"
// #include "msg.h"
extern thread_local MsgFactory msgFactory;
const char* msgName(int id);

struct instruction_report
{
    int err_code;
    std::string err_str;
    std::vector<std::string>  logMsgs;
    void update(Blake2bHasher &h) const
    {
        h.update(std::to_string(err_code));
        h.update(err_str);
        for(auto &s: logMsgs)
            h.update(s);
    }
};

inline outBuffer & operator<< (outBuffer& b,const instruction_report &s)
{
    b<<s.err_code<<s.err_str<<s.logMsgs;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  instruction_report &s)
{
    b>>s.err_code>>s.err_str>>s.logMsgs;
    return b;
}
struct transaction_report
{
    int err_code;
    std::string err_str;
    std::map<int, instruction_report> instruction_reports;
    void update(Blake2bHasher &h) const
    {
        h.update(std::to_string(err_code));
        h.update(err_str);
        for(auto& z: instruction_reports)
        {
            z.second.update(h);
        }
    }
};
inline outBuffer & operator<< (outBuffer& b,const transaction_report &s)
{
    b<<s.err_code<<s.err_str<<s.instruction_reports;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  transaction_report &s)
{
    b>>s.err_code>>s.err_str>>s.instruction_reports;
    return b;
}


namespace msgid
{
    enum MSG_ID
    {
        HeartBeatREQ=0,
        HeartBeatRSP=1,
        ValidateBlockREQ=2, 
        ValidateBlockRSP=3, 
        BlockInfo=4, 
        BlockAcceptedREQ=5,
        GetTransactionREQ=6,GetTransactionRSP=7,
        BlockDBStore=8, 
        DoHeartBeatREQ=9, 
        ConfirmLeaderREQ=10, ConfirmLeaderRSP=11,
        TX=12,
        attachment_data=13,
        GetUserNonceREQ=14, GetUserNonceRSP=15,

        LcEnvelopeREQ=16,
        DelayNotificationREQ=17,

        BlockDiffValidateREQ=18, BlockDiffValidateRSP=19,
        BlockAccepted2REQ = 20,
    };

}
