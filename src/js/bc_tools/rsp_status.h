#pragma once

    // Базовый статус для всех RSP: errcode + errstr, found() вычисляется.
    // errcode: 0 = OK, 404 = not found, 503 = syncing, 500 = internal.
namespace msgcode {
    constexpr int OK        = 0;
    constexpr int NOT_FOUND = 404;
    constexpr int INTERNAL  = 500;
    constexpr int SYNCING   = 503;
}
struct rsp_status
{
    int32_t errcode = 0;
    std::string errstr;

    bool found() const { return errcode == 0; }
    bool ok() const { return errcode == 0; }

    void setError(int code) { errcode = code; errstr.clear(); }
    void setError(int code, const std::string& msg) { errcode = code; errstr = msg; }
    void setOk() { errcode = 0; errstr.clear(); }

    void update_status(Blake2bHasher& h) const
    {
        h.update(std::to_string(errcode));
        h.update(errstr);
    }
    void pack_status(outBuffer& b) const
    {
        b<<errcode<<errstr;
    }
    void unpack_status(inBuffer& b)
    {
        b>>errcode>>errstr;
    }
};

