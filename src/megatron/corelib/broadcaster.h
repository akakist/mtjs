#pragma once
#include "IInstance.h"

/**
* Базовый класс для сервисов, дающим им возможность посылки сообщений
*/
class Broadcaster
{
public:
    Broadcaster(IInstance* _if):__BRDifa(_if) {}
    /// послать евент в сервис
    void sendEvent(const SERVICE_id& svs, const REF_getter<Event::Base>&e);
    void sendEvent(ListenerBase *svs, const REF_getter<Event::Base>&e);
    /// послать евент в удаленный сервис по RPC
    void sendEvent(const msockaddr_in& addr, const SERVICE_id& svs, const REF_getter<Event::Base>&e);
    /// послать евент в удаленный сервис по RPC
    void sendEvent(const std::string& addr, const SERVICE_id& svs, const REF_getter<Event::Base>&e);
    /// послать евент обратно по маршруту
    void passEvent(const REF_getter<Event::Base>&e);
    IInstance* getIInstance()
    {
        return __BRDifa;
    }

private:
    IInstance* __BRDifa;

};
