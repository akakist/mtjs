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
        HeartBeatREQ,HeartBeatRSP,
        LeaderCertificate, ValidateBlockREQ, ValidateBlockRSP, BlockInfo, BlockAcceptedREQ,BlockAcceptedRSP, GetTransactionREQ,GetTransactionRSP,
        BlockDBStore, GetSavedBlocksREQ,GetSavedBlocksRSP, DoHeartBeatREQ, ConfirmLeaderREQ, ConfirmLeaderRSP,
        TX,
        attachment_data,
        GetUserStatusREQ,
        GetUserStatusRSP,
        LcEnvelopeREQ,
	DoYouHaveBlockREQ,
	DoYouHaveBlockRSP
    };

}
