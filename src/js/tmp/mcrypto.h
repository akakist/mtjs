#pragma once
#include <string>
namespace mcrypto
{
    std::string hash(const std::string & buf);
    std::pair<std::string, std::string> generate_keypair();
    std::string sign_detached(const std::string & message, const std::string & sk);
    bool verify_detached(const std::string & signature, const std::string & message, const std::string & pk);

}