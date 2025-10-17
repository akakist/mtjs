#pragma once


#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "msockaddr_in.h"
namespace ServiceEnum
{
    const SERVICE_id Telnet (genum_Telnet);

}







namespace telnetEventEnum
{
    // const EVENT_id RegisterType(genum_telnetRegisterType);
    const EVENT_id RegisterCommand(genum_telnetRegisterCommand);
    const EVENT_id Reply(genum_telnetReply);
    const EVENT_id DoListen(genum_telnetDoListen);
    const EVENT_id CommandEntered(genum_telnetCommandEntered);
}




namespace telnetEvent
{

    class Reply: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        Reply(const SOCKET_id& s,const std::string &_buffer, const route_t& r)
            :NoPacked(telnetEventEnum::Reply,r),socketId(s),buffer(_buffer) {}
        SOCKET_id socketId;
        std::string buffer;

    };

    class DoListen: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        DoListen(const msockaddr_in &_sa, const std::string &dname, const route_t& r)
            :NoPacked(telnetEventEnum::DoListen,r),sa(_sa),deviceName(dname) {}
        msockaddr_in sa;
        std::string deviceName;

    };





    class RegisterCommand: public Event::NoPacked
    {
        /**
        *   Регистрация команды
        *   Направление - от клиента
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        RegisterCommand(const std::string &_directory, const std::string& _cmd, const std::string& _help, const route_t&r):
            NoPacked(telnetEventEnum::RegisterCommand,r),directory(_directory),cmd(_cmd),help(_help) {}
        std::string directory;
        std::string cmd;
        std::string help;

    };





    class CommandEntered: public Event::NoPacked
    {
        /**
        *   Регистрация команды
        *   Направление - от сервера
        *   \param socketId  идентидификатор соединения
        *   \param tokens массив принятых слов
        */
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        CommandEntered(const SOCKET_id& s,const std::string &_command, const std::string& _path, const route_t& r)
            :NoPacked(telnetEventEnum::CommandEntered,r),socketId(s),command(_command), path(_path) {}

        SOCKET_id socketId;
        std::string command;
        std::string path;


    };



};

