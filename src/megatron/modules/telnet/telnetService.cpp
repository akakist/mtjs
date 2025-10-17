#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <version_mega.h>

#endif
#include "telnetService.h"
#include "telnet.h"
#include "telnet_keys.h"
#include "mutexInspector.h"
#include "Events/System/Net/socketEvent.h"
extern "C" {
#include <regex.h>
}
#include "Events/Tools/telnetEvent.h"


#include "Events/System/Net/socketEvent.h"
#include "events_telnet.hpp"

namespace vt100
{
    std::string attribute_reset()
    {
        return "\033[0m";
    }
    std::string attribute_bright()
    {
        return "\033[1m";
    }
    std::string attribute_dim()
    {
        return "\033[2m";
    }
    std::string attribute_underscore()
    {
        return "\033[4m";
    }
    std::string attribute_blink()
    {
        return "\033[5m";
    }
    std::string attribute_reverse()
    {
        return "\033[7m";
    }
    std::string attribute_hidden()
    {
        return "\033[8m";
    }
    std::string erase_line()
    {
        return "\033[2K";
    }
    std::string clear_screen()
    {
        return "\033[2J";
    }
    std::string cursor_save()
    {
        return "\0337";
    }
    std::string cursor_restore()
    {
        return "\0338";
    }
    std::string cursor_up()
    {
        return "\033[A";
    }
    std::string cursor_down()
    {
        return "\033[B";
    }
    std::string cursor_forward()
    {
        return "\033[C";
    }
    std::string cursor_forward(int n)
    {
        char s[100];
        snprintf(s,sizeof(s),"\033[%dC",n);
        return s;
    }
    std::string cursor_back(int n)
    {
        char s[100];
        snprintf(s,sizeof(s),"\033[%dD",n);
        return s;
    }
    std::string cursor_back()
    {
        return "\033[D";
    }
    std::string cursor_home()
    {
        return "\033[H";
    }
    std::string erase()
    {
        return "\033[P";
    }
    std::string insert_mode()
    {
        return "\033[4h";
    }
    std::string replace_mode()
    {
        return "\033[4l";
    }
}

static size_t strmax(const std::string& a, const std::string &b)
{
    size_t i=0;
    for(; i< a.size()&& i< b.size(); i++)
    {
        if(a[i]!=b[i]) return i;
    }
    return i;
}
static std::string getAutoAppendString(const std::vector<std::string> &v, const std::string& templ)
{
    MUTEX_INSPECTOR;
    std::vector<std::string> vm;
    for(size_t i=0; i<v.size(); i++)
    {
        if(v[i].find(templ)==0)
            vm.push_back(v[i]);
    }
    if(!vm.empty())
    {
        size_t max=vm[0].size();

        for(size_t i=0; i<vm.size(); i++)
        {
            size_t val=strmax(vm[0],vm[i]);
            if(val< max) max=val;
        }

        return vm[0].substr(templ.size(),max-templ.size());
    }
    return "";
}
bool Telnet::Service::DoListen(const telnetEvent::DoListen*e)
{
    SOCKET_id sid=iUtils->getNewSocketId();
    // msockaddr_in sa=i;
    SECURE sec;
    sec.use_ssl=false;
    m_deviceName=e->deviceName;
    sendEvent(ServiceEnum::Socket,new socketEvent::AddToListenTCP(sid,e->sa,"TELNET",false,sec,ListenerBase::serviceId));

    return true;
}

// void Telnet::Service::doListen()
// {
//     MUTEX_INSPECTOR;

//     XTRY;
//     for(auto &i:m_bindAddr)
//     {
//         SOCKET_id sid=iUtils->getNewSocketId();
//         msockaddr_in sa=i;
//         SECURE sec;
//         sec.use_ssl=false;
//         sendEvent(ServiceEnum::Socket,new socketEvent::AddToListenTCP(sid,sa,"TELNET",false,sec,ListenerBase::serviceId));
//     }
//     XPASS;
// }

Telnet::Service::Service(const SERVICE_id& id, const std::string& nm, IInstance* ifa):
    UnknownBase(nm),Broadcaster(ifa),
    ListenerBuffered1Thread(nm,id),
    stuff(new __telnet_stuff),iInstance(ifa)
{
    XTRY;

    XPASS;
}
bool Telnet::Service::on_startService(const systemEvent::startService*)
{
    {
        // doListen();
    }

    return true;
}


bool Telnet::Service::on_Accepted(const socketEvent::Accepted* evt)
{
    MUTEX_INSPECTOR;
    XTRY;
    REF_getter<Telnet::Session> c=new Telnet::Session(cmdEntries.root(),evt->esi);
    stuff->insert(evt->esi->id_,c);

    evt->esi->write_("\377\375\042\377\373\001"+vt100::insert_mode());


    char s[100];
    snprintf(s,sizeof(s),"%c%c%c",IAC,DO,TELOPT_NAWS);
    evt->esi->write_(s);

    prompt(c,evt->esi);
    XPASS;
    return true;
}





void popprint2(std::deque<unsigned char>&chars)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!chars.empty())
    {
        DBG(printf("%d ",chars[0]));
        chars.pop_front();
    }
    if(!chars.empty())
    {
        DBG(printf("%d ",chars[0]));
        chars.pop_front();
    }
    XPASS;
}
bool Telnet::Service::on_StreamRead(const socketEvent::StreamRead* evt)
{
    MUTEX_INSPECTOR;
    XTRY;
    REF_getter<Telnet::Session> W=stuff->get(evt->esi->id_);
    if (!W.valid())
    {
        logErr2("socketStreamRead socket not found");
        return true;
    }
    REF_getter<epoll_socket_info> esi=evt->esi;

    std::string in=evt->esi->inBuffer_.extract_all();
    std::deque<unsigned char> chars;
    for(size_t i=0; i<in.size(); i++)
    {
        chars.push_back(in[i]);
    }


    while(chars.size())
    {
        unsigned char cmd1=chars[0];
        if(cmd1!=IAC) break;
        chars.pop_front();
        switch(cmd1)
        {
        case IAC:
        {
            DBG(printf("IAC "));
            if(chars.size())
            {
                unsigned char cmd2=chars[0];
                chars.pop_front();
                switch(cmd2)
                {
                case WILL:
                    DBG(printf("WILL "));
                    {
                        if(chars.size())
                        {
                            unsigned char cmd3=chars[0];
                            chars.pop_front();
                            switch(cmd3)
                            {
                            case TELOPT_LINEMODE:
                                DBG(printf("TELOPT_LINEMODE "));
                                break;
                            case TELOPT_SGA:
                                DBG(printf("TELOPT_SGA "));
                                break;
                            case TELOPT_ECHO:
                                DBG(printf("TELOPT_ECHO "));
                                break;
                            default:
                                DBG(printf("ND3(%d)",cmd3));

                            }
                        }
                    }
                    break;
                case WONT:
                    DBG(printf("WONT "));
                    {
                        if(chars.size())
                        {
                            unsigned char cmd3=chars[0];
                            chars.pop_front();
                            switch(cmd3)
                            {
                            case TELOPT_LINEMODE:
                                DBG(printf("TELOPT_LINEMODE "));
                                break;
                            case TELOPT_SGA:
                                DBG(printf("TELOPT_SGA "));
                                break;
                            case TELOPT_ECHO:
                                DBG(printf("TELOPT_ECHO "));
                                break;
                            default:
                                DBG(printf("ND3(%d)",cmd3));

                            }
                        }

                    }
                    break;
                case DONT:
                    DBG(printf("DONT "));
                    {
                        if(chars.size())
                        {
                            unsigned char cmd3=chars[0];
                            chars.pop_front();
                            switch(cmd3)
                            {
                            case TELOPT_LINEMODE:
                                DBG(printf("TELOPT_LINEMODE "));
                                break;
                            case TELOPT_SGA:
                                DBG(printf("TELOPT_SGA "));
                                break;
                            case TELOPT_ECHO:
                                DBG(printf("TELOPT_ECHO "));
                                break;
                            default:
                                DBG(printf("ND3(%d)",cmd3));

                            }
                        }
                    }
                    break;
                case DO:
                    DBG(printf("DO "));
                    {
                        if(chars.size())
                        {
                            unsigned char cmd3=chars[0];
                            chars.pop_front();
                            switch(cmd3)
                            {
                            case TELOPT_LINEMODE:
                                DBG(printf("TELOPT_LINEMODE "));
                                break;
                            case TELOPT_SGA:
                                DBG(printf("TELOPT_SGA "));
                                break;
                            case TELOPT_ECHO:
                                DBG(printf("TELOPT_ECHO "));
                                break;
                            default:
                                DBG(printf("ND3(%d)",cmd3));

                            }
                        }
                    }
                    break;
                case SB:
                    DBG(printf("SB "));
                    {
                        if(chars.size())
                        {
                            unsigned char cmd3=chars[0];
                            chars.pop_front();
                            switch(cmd3)
                            {
                            case TELOPT_LINEMODE:
                                DBG(printf("TELOPT_LINEMODE "));
                                {
                                    if(chars.size())
                                    {
                                        unsigned char cmd4=chars[0];
                                        chars.pop_front();
                                        switch(cmd4)
                                        {
                                        case LM_MODE:
                                            DBG(printf("LM_MODE "));
                                            break;
                                        case LM_FORWARDMASK:
                                            DBG(printf("LM_FORWARDMASK "));
                                            break;
                                        case LM_SLC:
                                            DBG(printf("LM_SLC "));
                                            while(chars.size())
                                            {
                                                if(chars.size()>=2)
                                                {
                                                    if(chars[0]==IAC && chars[1]==SE)
                                                    {
                                                        if(chars.size())chars.pop_front();
                                                        if(chars.size())chars.pop_front();
                                                        DBG(printf("IAC SE finish"));
                                                        break;
                                                    }
                                                }
                                                unsigned char cmd5=chars[0];
                                                chars.pop_front();
                                                switch(cmd5)
                                                {
                                                case SLC_SYNCH:
                                                    DBG(printf("SLC_SYNCH "));
                                                    {
                                                        if(chars.size())chars.pop_front();
                                                        if(chars.size())chars.pop_front();
                                                    }
                                                    break;

                                                case SLC_BRK:
                                                    DBG(printf("SLC_BRK "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_IP:
                                                    DBG(printf("SLC_IP "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_AO:
                                                    DBG(printf("SLC_AO "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_AYT:
                                                    DBG(printf("SLC_AYT "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_EOR:
                                                    DBG(printf("SLC_EOR "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_ABORT:
                                                    DBG(printf("SLC_ABORT "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_EOF:
                                                    DBG(printf("SLC_EOF "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_SUSP:
                                                    DBG(printf("SLC_SUSP "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_EC:
                                                    DBG(printf("SLC_EC "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_EL:
                                                    DBG(printf("SLC_EL "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_EW:
                                                    DBG(printf("SLC_EW "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_RP:
                                                    DBG(printf("SLC_RP "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_LNEXT:
                                                    DBG(printf("SLC_LNEXT "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_XON:
                                                    DBG(printf("SLC_XON "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_XOFF:
                                                    DBG(printf("SLC_XOFF "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_FORW1:
                                                    DBG(printf("SLC_FORW1 "));
                                                    popprint2(chars);
                                                    break;
                                                case SLC_FORW2:
                                                    DBG(printf("SLC_FORW2 "));
                                                    popprint2(chars);
                                                    break;
                                                default:
                                                    DBG(printf("ND5(%d) ",cmd5));

                                                }

                                            }
                                            break;
                                        default:
                                            DBG(printf("ND4(%d)",cmd4));

                                        }
                                    }
                                }
                                break;
                            case TELOPT_SGA:
                                DBG(printf("TELOPT_SGA "));
                                break;
                            case TELOPT_ECHO:
                                DBG(printf("TELOPT_ECHO "));
                                break;
                            case TELOPT_BINARY:
                                DBG(printf("TELOPT_BINARY "));
                                break;
                            case TELOPT_RCP:
                                DBG(printf("TELOPT_RCP "));
                                break;
                            case TELOPT_NAMS:
                                DBG(printf("TELOPT_NAMS "));
                                break;
                            case TELOPT_STATUS:
                                DBG(printf("TELOPT_STATUS "));
                                break;
                            case TELOPT_TM:
                                DBG(printf("TELOPT_TM "));
                                break;
                            case TELOPT_RCTE:
                                DBG(printf("TELOPT_RCTE "));
                                break;
                            case TELOPT_NAOL:
                                DBG(printf("TELOPT_NAOL "));
                                break;
                            case TELOPT_NAOP:
                                DBG(printf("TELOPT_NAOP "));
                                break;
                            case TELOPT_NAOCRD:
                                DBG(printf("TELOPT_NAOCRD "));
                                break;
                            case TELOPT_NAOHTS:
                                DBG(printf("TELOPT_NAOHTS "));
                                break;
                            case TELOPT_NAOHTD:
                                DBG(printf("TELOPT_NAOHTD "));
                                break;
                            case TELOPT_NAOFFD:
                                DBG(printf("TELOPT_NAOFFD "));
                                break;
                            case TELOPT_NAOVTS:
                                DBG(printf("TELOPT_NAOVTS "));
                                break;
                            case TELOPT_NAOVTD:
                                DBG(printf("TELOPT_NAOVTD "));
                                break;
                            case TELOPT_NAOLFD:
                                DBG(printf("TELOPT_NAOLFD "));
                                break;
                            case TELOPT_XASCII:
                                DBG(printf("TELOPT_XASCII "));
                                break;
                            case TELOPT_LOGOUT:
                                DBG(printf("TELOPT_LOGOUT "));
                                break;
                            case TELOPT_BM:
                                DBG(printf("TELOPT_BM "));
                                break;
                            case TELOPT_DET:
                                DBG(printf("TELOPT_DET "));
                                break;
                            case TELOPT_SUPDUP:
                                DBG(printf("TELOPT_SUPDUP "));
                                break;
                            case TELOPT_SUPDUPOUTPUT:
                                DBG(printf("TELOPT_SUPDUPOUTPUT "));
                                break;
                            case TELOPT_SNDLOC:
                                DBG(printf("TELOPT_SNDLOC "));
                                break;
                            case TELOPT_TTYPE:
                                DBG(printf("TELOPT_TTYPE "));
                                break;
                            case TELOPT_EOR:
                                DBG(printf("TELOPT_EOR "));
                                break;
                            case TELOPT_TUID:
                                DBG(printf("TELOPT_TUID "));
                                break;
                            case TELOPT_OUTMRK:
                                DBG(printf("TELOPT_OUTMRK "));
                                break;
                            case TELOPT_TTYLOC:
                                DBG(printf("TELOPT_TTYLOC "));
                                break;
                            case TELOPT_3270REGIME:
                                DBG(printf("TELOPT_3270REGIME "));
                                break;
                            case TELOPT_X3PAD:
                                DBG(printf("TELOPT_X3PAD "));
                                break;
                            case TELOPT_NAWS:
                                DBG(printf("TELOPT_NAWS "));
                                {
                                    int x1=0,x2=0,y1=0,y2=0;
                                    if(chars.size())
                                    {
                                        x1=chars[0];
                                        chars.pop_front();
                                    }
                                    if(chars.size())
                                    {
                                        x2=chars[0];
                                        chars.pop_front();
                                    }
                                    if(chars.size())
                                    {
                                        y1=chars[0];
                                        chars.pop_front();
                                    }
                                    if(chars.size())
                                    {
                                        y2=chars[0];
                                        chars.pop_front();
                                    }
                                    int x=x1<<8|x2;
                                    int y=y1<<8|y2;
                                    {
                                        W->width=x;
                                        W->height=y;
                                    }
                                }
                                break;
                            case TELOPT_TSPEED:
                                DBG(printf("TELOPT_TSPEED "));
                                break;
                            case TELOPT_LFLOW:
                                DBG(printf("TELOPT_LFLOW "));
                                break;
                            case TELOPT_XDISPLOC:
                                DBG(printf("TELOPT_XDISPLOC "));
                                break;
                            case TELOPT_OLD_ENVIRON:
                                DBG(printf("TELOPT_OLD_ENVIRON "));
                                break;
                            case TELOPT_AUTHENTICATION:
                                DBG(printf("TELOPT_AUTHENTICATION "));
                                break;
                            case TELOPT_ENCRYPT:
                                DBG(printf("TELOPT_ENCRYPT "));
                                break;
                            case TELOPT_NEW_ENVIRON:
                                DBG(printf("TELOPT_NEW_ENVIRON "));
                                break;
                            case TELOPT_EXOPL:
                                DBG(printf("TELOPT_EXOPL "));
                                break;
                            default:
                                DBG(printf("ND3(%d)",cmd3));

                            }
                        }

                    }
                    break;
                case GA:
                    DBG(printf("GA "));
                    break;
                case EL:
                    DBG(printf("EL "));
                    break;
                case EC:
                    DBG(printf("EC "));
                    break;
                case AYT:
                    DBG(printf("AYT "));
                    break;
                case AO:
                    DBG(printf("AO "));
                    break;
                case IP:
                    DBG(printf("IP "));
                    break;
                case BREAK:
                    DBG(printf("BREAK "));
                    break;
                case DM:
                    DBG(printf("DM "));
                    break;
                case NOP:
                    DBG(printf("NOP "));
                    break;
                case SE:
                    DBG(printf("SE "));
                    break;
                case EOR:
                    DBG(printf("EOR "));
                    break;
                case ABORT:
                    DBG(printf("ABORT "));
                    break;
                case SUSP:
                    DBG(printf("SUSP "));
                    break;
                case xEOF:
                    DBG(printf("xEOF "));
                    break;
                default :
                    DBG(printf("ND2(%d) ",cmd2));


                }
            }
        }
        break;
        default :
            DBG(printf("ND1(%d) ",cmd1));
        }
    }
    std::string out;
    for(size_t i=0; i<chars.size(); i++)
    {
        out+=chars[i];
    }

    if(out==TKEY_F1) {}
    else if(out==TKEY_F2) {}
    else if(out==TKEY_F3) {}
    else if(out==TKEY_F4) {}
    else if(out==TKEY_F5) {}
    else if(out==TKEY_F6) {}
    else if(out==TKEY_F7) {}
    else if(out==TKEY_F8) {}
    else if(out==TKEY_F9) {}
    else if(out==TKEY_F10) {}
    else if(out==TKEY_F11) {}
    else if(out==TKEY_F12) {}
    else if(out==TKEY_INSERT)
    {
        bool mode;

        W->insertMode=!W->insertMode;
        mode=W->insertMode;

        if(mode)
            esi->write_(vt100::insert_mode());
        else
            esi->write_(vt100::replace_mode());
    }
    else if(out==TKEY_BS1 || out==TKEY_BS2)
    {
        if(W->on_TKEY_BS())
        {
            esi->write_(vt100::cursor_back()+vt100::erase());
        }
    }
    else if(out==TKEY_DELETE)
    {

        bool ok=false;
        {
            if(W->curpos < (int)W->mx_current_command_line.size())
            {
                W->mx_current_command_line=
                    W->mx_current_command_line.substr(0,W->curpos)
                    +W->mx_current_command_line.substr(W->curpos+1,W->mx_current_command_line.size()-W->curpos-1);
                ok=true;
            }
        }
        if(ok)
        {
            esi->write_(vt100::erase());
        }

    }
    else if(out==TKEY_END || out==TKEY_END2)
    {
    }
    else if(out==TKEY_HOME || out==TKEY_HOME2)
    {
    }
    else if(out==TKEY_LEFT) // MULTILINE OK
    {
        int cp;
        {
            cp=W->curpos;
            if(W->curpos>0)
            {
                W->curpos--;
                //    ok=true;
            }
        }
        if(cp>0)
        {
            if(cp%W->width==0 && cp!=0)
            {

                esi->write_(vt100::cursor_up()+vt100::cursor_forward(W->width-1));

            }
            else
            {
                esi->write_(out);
            }
        }

    }
    else if(out==TKEY_RIGHT) // MULTILINE OK
    {
        bool ok=false;
        int w,cp;
        {
            w=W->width;
            if(W->curpos<(int)W->mx_current_command_line.size())
            {
                W->curpos++;
                ok=true;
            }
            cp=W->curpos;
        }
        if(ok)
        {
            if(cp%w==0)
            {

                esi->write_(vt100::cursor_down()+vt100::cursor_back(w-1));
            }
            else
            {
                esi->write_(out);
            }
        }
    }
    else if(out==TKEY_UP || out==TKEY_DOWN)
    {
        std::string cmd;
        bool ok=false;
        {
            if(W->mx_command_history.size())
            {
                int next=0;
                if(out==TKEY_UP)
                {
                    next=-1;
                }
                else
                {
                    next=1;
                }

                int idx=W->mx_command_history.size()+W->command_history_pos+next;
                if(idx>=0 && idx<(int)W->mx_command_history.size())
                {
                    W->mx_current_command_line=W->mx_command_history[W->mx_command_history.size()+W->command_history_pos+next];
                    W->curpos=W->mx_current_command_line.size();
                    cmd=W->mx_current_command_line;
                    ok=true;
                    W->command_history_pos+=next;
                }
                else
                {
                    cmd="";
                    ok=true;
                    W->curpos=0;
                }

            }

        }
        if(ok)
        {
            esi->write_(vt100::erase_line()+vt100::cursor_back(promptString(W).size()+W->curpos+100));
            prompt(W,esi);
            esi->write_(cmd);

        }
    }
    else if(out==TKEY_PGDN) {}
    else if(out==TKEY_PGUP) {}
    else if(out==TKEY_ESC)
    {
        esi->write_(vt100::erase_line()+vt100::cursor_back(promptString(W).size()+W->curpos+100));
        prompt(W,esi);
    }
    else if(out==TKEY_TAB)
    {
        std::deque<std::string> v;
        std::string norm_cmdline;
        {
            norm_cmdline=W->mx_current_command_line;
        }
        REF_getter<Telnet::Node> N=W->defaultNode();
        auto  _c=N->commands();

        std::vector<std::string> commands;

        for(auto& i:_c)
        {
            // std::string templ=i.first;
            commands.push_back(i.first);
        }
        std::map<std::string,REF_getter<Node> > _n=N->children();
        for(auto& i:_n)
        {
            commands.push_back(i.second->name);
        }
        std::string append=getAutoAppendString(commands,norm_cmdline);



        esi->write_(append);
        {
            W->mx_current_command_line+=append;
        }
        W->curpos+=append.size();
    }
    else
    {
        if(out.size())
        {
            switch(out[0])
            {
            case 3:
            {
                esi->write_and_close("^C\r\n");
            }
            break;
            case 9: // TAB
                break;
            case 13: // ENTER
                DBG(logErr2("ENTER"));
                processCommand(W,esi);
                break;
            default:
            {
                std::string toprint;
                {
                    if(W->curpos<0)W->curpos=0;
                    if(W->curpos > (int)W->mx_current_command_line.size()) W->curpos=W->mx_current_command_line.size();
                    if(W->insertMode)
                    {
                        W->mx_current_command_line=W->mx_current_command_line.substr(0,W->curpos)+out[0]+W->mx_current_command_line.substr(W->curpos,W->mx_current_command_line.size()-W->curpos);
                        toprint=std::string("")+out[0];
                        W->curpos++;
                    }
                    else
                    {
                        if(W->curpos==(int)W->mx_current_command_line.size())
                            W->mx_current_command_line=W->mx_current_command_line+out[0];
                        else
                        {
                            W->mx_current_command_line[W->curpos]=out[0];
                        }
                        W->curpos++;
                        toprint=std::string("")+out[0];
                    }
                }
                if(toprint.size())
                    esi->write_(toprint);
            }
            break;
            }
        }
    }

    return true;
    XPASS;
}
std::string Telnet::Service::promptString(const REF_getter<Telnet::Session>& W)
{
    std::string &dn=m_deviceName;
    return dn+":"+W->defaultNode()->path()+"$ ";

}
void Telnet::Service::prompt(const REF_getter<Telnet::Session>& W, const REF_getter<epoll_socket_info>&esi)
{
    esi->write_(promptString(W));
}
bool Telnet::Service::paramMatch(const std::string & regexp, const std::string & cmd)
{
    // std::string regexp=cmdEntries.getRegex(param);

    bool ok=false;
    int err;
    regex_t re;
    err = regcomp(&re, regexp.c_str(), REG_EXTENDED);
    regmatch_t pm;
    ::memset(&pm, 0, sizeof(regmatch_t));
    if (err)
    {
        char erbuf[111];
        regerror(err, &re, erbuf, sizeof(erbuf));
        throw CommonError("regex '%s' %s",regexp.c_str(),erbuf);
        return false;
    }
    if (!regexec(&re, cmd.c_str(), 1, &pm, 0))
    {
        if (pm.rm_so != -1)
        {
            size_t matchlen = pm.rm_eo - pm.rm_so;
            if(matchlen==cmd.size()) ok=true;
            else
            {
                DBG(logErr2("paramMatch: matchlen!= buffer.size"));
            }
        }
    }
    regfree(&re);
    // DBG(logErr2("paramMatch %s %s %d",param.c_str(),exp.c_str(),ok));
    return ok;
    return true;
}
static std::string bright(const std::string &s)
{
    return vt100::attribute_bright()+s+vt100::attribute_reset();
}
void Telnet::Service::processCommand(const REF_getter<Telnet::Session>& W, const REF_getter<epoll_socket_info>&esi)
{
    MUTEX_INSPECTOR;
    esi->write_("\r\n");
    std::string cmd;
    {
        cmd=W->mx_current_command_line;
        W->mx_current_command_line="";
        W->mx_command_history.push_back(cmd);
        W->command_history_pos=0;
    }
    // logErr2("processCommand: %s",cmd.c_str());
    std::deque<std::string> tokens=iUtils->splitStringDQ(" \t",cmd);
    if(tokens.size()>0)
    {
        if(iUtils->strlower(tokens[0])=="quit")
        {
            esi->write_and_close("\r\n");
            return;
        }
        else if(iUtils->strlower(tokens[0])=="ls" || iUtils->strlower(tokens[0])=="help" )
        {
            if(tokens.size()>1)
            {

                // if(iUtils->strlower(tokens[1])=="types")
                // {
                //     std::map<std::string,std::string> m=cmdEntries.getHelpOnTypes();
                //     esi->write_("Registered types \r\n\r\n");
                //     for(auto& i:m)
                //     {
                //         esi->write_(bright(i.first)+" - "+i.second+"\r\n");
                //     }
                //     prompt(W,esi);
                //     return;
                // }
                // else if(iUtils->strlower(tokens[1])=="typex")
                // {
                //     std::map<std::string,std::string> m=cmdEntries.getHelpOnRe();
                //     esi->write_("Templates of registered types \r\n\r\n");
                //     for(auto& i:m)
                //     {
                //         esi->write_(bright(i.first)+" - "+i.second+"\r\n");
                //     }
                //     prompt(W,esi);
                //     return;

                // }
            }
            std::string path=W->defaultNode()->path();
            if(path.size()==0) path="/";
            esi->write_("Commands in '"+path+"'\r\n\r\n");
            esi->write_(bright("help")+" - display this page\r\n");
            // esi->write_(bright("help types")+" - display types of parameters\r\n");
            // esi->write_(bright("help typex")+" - display regex of parameters\r\n");
            esi->write_(bright("quit")+" - quit console\r\n");
            esi->write_(bright("..")+" - go to parent directory\r\n");

            auto children=W->defaultNode()->children();
            auto commands=W->defaultNode()->commands();
            for(auto& i:children)
            {
                esi->write_(bright(i.second->name)+" - "+i.second->help+"\r\n");
            }
            for(auto& i:commands)
            {
                // if(i.second->params.size())
                //     esi->write_(bright(i.second->cmd_re+" ["+iUtils->join(" ",i.second->params)+"]")+" - "+i.second->help+"\r\n");
                // else
                esi->write_(bright(i.second->cmd_re)+" - "+i.second->help+"\r\n");
            }
            prompt(W,esi);
            return;
        }
        else if(iUtils->strlower(tokens[0])=="cd")
        {
            if(tokens.size()>1)
            {
            }
        }
        else
        {

            // try directory
            {
                std::string dir=tokens[0];
                if(dir=="..")
                {
                    REF_getter<Telnet::Node> N=W->defaultNode()->parent;
                    if(N.valid())
                    {
                        W->defaultNode(N);
                        prompt(W,esi);
                        return;
                    }
                    else
                    {
                        prompt(W,esi);
                        return;
                    }
                }
                else
                {
                    REF_getter<Telnet::Node> N=W->defaultNode()->getSubDir(dir);
                    if(N.valid())
                    {
                        W->defaultNode(N);
                        prompt(W,esi);
                        return;
                    }
                }

            }


        }

    }
    auto commands=W->defaultNode()->commands();
    auto path=W->defaultNode()->path();
    for(auto& i:commands)
    {
        Telnet::_Command *c=i.second.___ptr;
        if(paramMatch(i.second->cmd_re,cmd))
        {
            // logErr2("matched command %s",i.second->cmd_re.c_str());
            {
                XTRY;
                route_t r=c->dstService;
                passEvent(new telnetEvent::CommandEntered(esi->id_,cmd, path,r));
                return;
                XPASS;
            }
        }
        // else logErr2("not matched command %s",i.second->cmd_re.c_str());



    }

    if(cmd.size())
        esi->write_(cmd+": invalid command\r\n");
    prompt(W,esi);
}

void Telnet::Service::__telnet_stuff::erase(const SOCKET_id& id)
{
    MUTEX_INSPECTOR;
    XTRY;
    sessions.erase(id);
    XPASS;
}
REF_getter<Telnet::Session> Telnet::Service::__telnet_stuff::get(const SOCKET_id& id)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto i=sessions.find(id);
    if (i!=sessions.end()) return i->second;
    XPASS;
    return NULL;
}
void Telnet::Service::__telnet_stuff::insert(const SOCKET_id& id,const REF_getter<Telnet::Session> &C)
{
    MUTEX_INSPECTOR;
    XTRY;
    sessions.insert(std::make_pair(id,C));
    XPASS;
}
std::map<SOCKET_id,REF_getter<Telnet::Session> > Telnet::Service::__telnet_stuff::getContainer()
{
    MUTEX_INSPECTOR;
    XTRY;
    return sessions;
    XPASS;
}

Telnet::Session::Session(const REF_getter<Node> &N, const REF_getter<epoll_socket_info> &_esi)
    :mx_defaultNode(N),curpos(0),command_history_pos(0), insertMode(1),width(80),height(25),esi(_esi)
{
}
// bool Telnet::Service::on_RegisterType(const telnetEvent::RegisterType* e)
// {
//     cmdEntries.registerType(e->name,e->pattern,e->help);
//     return true;

// }
bool Telnet::Service::on_RegisterCommand(const telnetEvent::RegisterCommand* e)
{
    MUTEX_INSPECTOR;
    std::deque<std::string> d=iUtils->splitStringDQ("/",e->directory);
    std::deque<std::string> dbase;
    std::string dname;
    for(size_t i=0; i<d.size(); i++)
    {
        cmdEntries.registerDirectory(dbase,d[i],"Directory");
        dbase.push_back(d[i]);
    }

    cmdEntries.registerCommand(d,e->cmd,e->help,e->route);
    return true;

}
bool Telnet::Service::on_Reply(const telnetEvent::Reply* e)
{
    MUTEX_INSPECTOR;
    REF_getter<Telnet::Session> W=stuff->get(e->socketId);
//    REF_getter<epoll_socket_info> esi=stuff->getOrNull(e->socketId);
    REF_getter<epoll_socket_info>esi=W->esi;
    if (!W.valid() || !esi.valid())
    {
        logErr2("telnetReply user not found");
        return true;
    }
    std::string buf;
    for(size_t i=0; i<e->buffer.size(); i++)
    {
        if(e->buffer[i]=='\n') buf+='\r';
        buf+=e->buffer[i];
    }

    esi->write_(bright(buf+"\r\n"));

    prompt(W,esi);
    return true;

}

bool Telnet::Session::on_TKEY_BS()
{
    bool ok=false;
    if(curpos>0)
    {
        if(curpos>0)
        {
            curpos--;
        }
        if(curpos < (int)mx_current_command_line.size())
        {
            mx_current_command_line=
                mx_current_command_line.substr(0,curpos)
                +mx_current_command_line.substr(curpos+1,mx_current_command_line.size()-curpos-1);
            ok=true;
        }
    }
    return ok;

}
REF_getter<Telnet::Node> Telnet::Node::getSubDir(const std::string& dir)
{
    auto i=m_children.find(dir);
    if(i==m_children.end())
        return NULL;
    return i->second;
}
void Telnet::Node::addChildNode(const std::string& section,const REF_getter<Node>&n)
{
    if(!m_children.count(section))
    {
        m_children.insert(std::make_pair(section,n));
    }
}
void Telnet::Node::addCommand(const std::string& cmd,const REF_getter<_Command>&n)
{
    if(m_commands.count(cmd)) logErr2("addCommand: command '%s' already exists",cmd.c_str());
    m_commands.erase(cmd);
    m_commands.insert(std::make_pair(cmd,n));
}
std::map<std::string,REF_getter<Telnet::Node> > Telnet::Node::children()
{
    return m_children;
}
std::map<std::string,REF_getter<Telnet::_Command> > Telnet::Node::commands()
{
    return m_commands;
}
std::string Telnet::Node::path()
{
    std::deque<std::string> d;
    Node *n=this;
    d.push_front(this->name);
    while(n->parent)
    {
        d.push_front(n->parent->name);
        n=n->parent;
    }
    if(d.empty()) return "/";
    return iUtils->join("/",d);
}
REF_getter<Telnet::Node> Telnet::CommandEntries::root()
{
    return m_root;
}
void Telnet::CommandEntries::registerDirectory(const std::deque<std::string> &d, const std::string& name, const std::string& help)
{
    REF_getter<Node> n(root());
    for(size_t i=0; i<d.size(); i++)
    {
        n=n->getSubDir(d[i]);
        if(!n.valid()) throw CommonError("cannot find subdir '%s'",d[i].c_str());
    }
    REF_getter<Node> P(new Node(name,n.get(),help));
    n->addChildNode(name,P);
}
void Telnet::CommandEntries::registerCommand(const std::deque<std::string> &d, const std::string& cmd, const std::string& help,const route_t& dst)
{

    {
        // std::deque<std::string> pars=iUtils->splitStringDQ(" ",params);
        // {

        //     for(size_t i=0; i<pars.size(); i++)
        //     {
        //         if(mx_types.count(pars[i]))
        //         {
        //         }
        //         else
        //             throw CommonError("param type '%s' is not defined",pars[i].c_str());
        //     }
        // }
    }
    REF_getter<Node> n(root());
    for(const auto & i : d)
    {
        n=n->getSubDir(i);
        if(!n.valid()) throw CommonError("cannot find subdir '%s'",i.c_str());
    }
    route_t r=dst;
    r.pop_front();
    REF_getter<_Command> C(new _Command(r,cmd,help));
    n->addCommand(cmd,C);
}
// std::string Telnet::CommandEntries::getRegex(const std::string &param)
// {
//     std::string regex;
//     {
//         if(mx_types.count(param))
//         {
//             regex=mx_types[param].first;
//         }
//         else
//             throw CommonError("param type '%s' is not defined",param.c_str());
//     }
//     return regex;
// }
// std::map<std::string,std::string> Telnet::CommandEntries::getHelpOnTypes()
// {
//     std::map<std::string,std::string> m;
//     for(auto& i:mx_types)
//     {
//         m[i.first]=i.second.second;
//     }
//     return m;
// }
// std::map<std::string,std::string> Telnet::CommandEntries::getHelpOnRe()
// {
//     std::map<std::string,std::string> m;
//     for(auto& i:mx_types)
//     {
//         m[i.first]=i.second.first;
//     }
//     return m;
// }
bool Telnet::Service::on_NotifyBindAddress(const socketEvent::NotifyBindAddress*e)
{
    //if(iInstance->no_bind())
    //  throw CommonError("Telnet::Service::on_NotifyBindAddress %s",_DMI().c_str());

//    socklen_t len=e->esi->local_name.maxAddrLen();
//    if(getsockname(CONTAINER(e->esi->get_fd()),e->esi->local_name.addr(),&len))
//    {
//        logErr2("getsockname: errno %d %s",errno,strerror(errno));
//        return true;
//    }
    return true;
}
void registerTelnetService(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Telnet,"Telnet",getEvents_telnet());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Telnet,Telnet::Service::construct,"Telnet");
        regEvents_telnet();
    }

}

void Telnet::Service::__telnet_stuff::on_delete(const REF_getter<epoll_socket_info>&esi, const std::string& )
{
    sessions.erase(esi->id_);
}

bool Telnet::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    auto& ID=e->id;
    XTRY;
    if(socketEventEnum::NotifyOutBufferEmpty==ID)
        return true;
    if( socketEventEnum::Disaccepted==ID)
        return on_Disaccepted((const socketEvent::Disaccepted*)e.get());
    if( socketEventEnum::Disconnected==ID)
        return on_Disconnected((const socketEvent::Disconnected*)e.get());
    if( socketEventEnum::Accepted==ID)
        return on_Accepted((const socketEvent::Accepted*)e.get());
    if( socketEventEnum::StreamRead==ID)
        return on_StreamRead((const socketEvent::StreamRead*)e.get());
    if( socketEventEnum::NotifyBindAddress==ID)
        return on_NotifyBindAddress((const socketEvent::NotifyBindAddress*)e.get());
    // if( telnetEventEnum::RegisterType==ID)
    //     return(this->on_RegisterType((const telnetEvent::RegisterType*)e.get()));
    if( telnetEventEnum::RegisterCommand==ID)
        return(this->on_RegisterCommand((const telnetEvent::RegisterCommand*)e.get()));
    if( telnetEventEnum::Reply==ID)
        return(this->on_Reply((const telnetEvent::Reply*)e.get()));
    if( telnetEventEnum::DoListen==ID)
        return(this->DoListen((const telnetEvent::DoListen*)e.get()));
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());

    XPASS;
    return false;

}
bool Telnet::Service::on_Disaccepted(const socketEvent::Disaccepted*e)
{
    stuff->on_delete(e->esi,e->reason);
    return true;
}
bool Telnet::Service::on_Disconnected(const socketEvent::Disconnected*e)
{
    stuff->on_delete(e->esi,e->reason);
    return true;
}
