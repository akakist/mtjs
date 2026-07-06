#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
std::string load_file(const std::string & fn);
void processSources( FILE* fm);

using namespace std;
set<string> m_includes;
set<string> targets;
set<string> m_sources;
map <string, string> conf;
std::string m_cflags_include;
vector<string> m_outD,m_outR;
map <string,string> m_srcFiles;
vector<string> CC;
set < string > splitTOKs(const char *seps, const char *src);
vector < string > splitTOK(const char *seps, const char *src);
string find_srcfile(const string& s);
std::string strupper(const std::string &s);
std::string strlower(const std::string & s);

//string exename(const string &s,const string& ext);
//string relexename(const string &s,const string& ext);
vector < string > splitTOK(const char *seps, const char *src);
string replace_vals(const string& regex,const string& repl, const string &src);
bool is_file_exists(const string &pathname);
string join(const string &pattern, const vector<string> & arr);
string join(const char* pattern, const vector<string> & arr);
void check_path_wfn(const string & _pathname);
std::set<std::string> baseClasses;
std::string itostring(int i)
{

    char ss[200];
    std::string s;
    ::snprintf(ss, sizeof(ss) - 1, "%d", i);
    s = ss;
    return s;

}
std::string trim(const std::string &s)
{
    std::deque<char> d;
    for (unsigned k=0; k<s.size(); k++)
    {
        d.push_back(s[k]);
    }
    while (d.size())
    {
        if (isspace(*d.begin()) || *d.begin()=='-') d.pop_front();
        else break;
    }
    while (d.size())
    {
        if (isspace(*d.rbegin())|| *d.rbegin()=='-') d.pop_back();
        else break;
    }
    std::string ret;
    for (unsigned i=0; i<d.size(); i++)
        ret+=d[i];
    return ret;
}
std::string rtrim(const std::string &s)
{
    std::deque<char> d;
    for (unsigned k=0; k<s.size(); k++)
    {
        d.push_back(s[k]);
    }
    while (d.size())
    {
        if (isspace(*d.rbegin())|| *d.rbegin()=='-') d.pop_back();
        else break;
    }
    std::string ret;
    for (unsigned i=0; i<d.size(); i++)
        ret+=d[i];
    return ret;
}
void replace(string&s,const char old,const char newc)
{
    for(size_t i=0; i<s.size(); i++)
    {
        if(s[i]==old)s[i]=newc;
    }
}
std::string removeSpaces(const std::string& s)
{
    std::string r;
    for(size_t i=0; i<s.size(); i++)
    {
        if(!isspace(s[i])) r+=s[i];
    }
    return r;
}
std::string h_body,cpp_body,enum_body,reg_body;


void genclasses(const char * _fn)
{
    std::string fn;
    if(_fn)fn=_fn;
    std::string body=load_file(fn);

    int pz=fn.rfind('.');
    std::string h_fn=fn.substr(0,pz)+".h";
    std::string basefilename="KALL";
    std::vector<std::string> fv=splitTOK(".",fn.c_str());
    if(fv.size()) basefilename=fv[0];
    else return;

    std::string section;
    std::string title;
    std::string enumPrefix;
    bool inSection=false;
    std::map<std::string, std::vector<std::string> > els;

    std::vector<std::string> v=splitTOK("\r\n",body.c_str());

    for(size_t i=0; i< v.size(); i++)
    {

        if(v[i].size()>1)
        {

            if(v[i].substr(0,4)=="//!!")
            {
                std::vector<std::string> tv=splitTOK(" \t",v[i].substr(4,v[i].size()-4).c_str());
                if(tv.size()!=3) throw "if(tv.size()!=3)";
                title=trim(tv[0]);
                basefilename=trim(tv[1]);
                enumPrefix=trim(tv[2]);
            }
            else if(v[i].substr(0,4)=="//!<")
            {
                section=trim(v[i].substr(4,v[i].size()-4));
                printf("section:%s\n",section.c_str());
                inSection=true;
            }
            else if(v[i].substr(0,4)=="//!>")
            {
                section="";
                inSection=false;
                printf("!section:%s\n",section.c_str());
            }
            else
            {
                if(inSection)
                {
                    std::vector<std::string> w=splitTOK("=, \t",v[i].c_str());
                    if(w.size())
                    {
                        els[section].push_back(w[0]);
                        printf("%s",w[0].c_str());
                    }
                }
            }
        }
    }
    for(std::map<std::string, std::vector<std::string> >::iterator i=els.begin(); i!=els.end(); i++)
    {
        section=i->first;
        std::string fn_h=basefilename+section+"IFace"+".h";
        std::string fn_cpp=basefilename+section+"IFace"+".cpp";
        std::string buf_h,buf_cpp;
        std::string DEF="__________IFace___"+title+section+"__autogen__________HHH";
        buf_h+="/* automatically generated source. DO NOT EDIT!!! */\n";
        buf_h+="#ifndef "+DEF+"\n";
        buf_h+="#define "+DEF+"\n";

        buf_h+="#include \"listenerBase.h\"\n";
        buf_h+="#include \"version_mega.h\"\n";
        buf_h+="#include \""+fn+"\"\n";
        buf_h+=(std::string)
               "\n\
namespace "+title+"\n\
{\n\
    class Interface"+i->first+"\n\
    {\n\
    protected:\n\
        Interface"+section+"(ListenerBase* l)\n\
        {\n\
             l->handlers.push_back(std::make_pair(handle"+title+""+section+",this));\n\
             regEvents();\n";
        buf_h+="\n\
        }\n";
        buf_h+="\n\
        public:\n\
        virtual ~Interface"+i->first+"(){}\n\
	static void getEvents(std::set<int> & s)\n\
	{\n\
	\n\
	    ";
        for(std::map<std::string, std::vector<std::string> >::iterator ii=els.begin(); ii!=els.end(); ii++)
        {

            for(size_t j=0; j<ii->second.size(); j++)
            {
                buf_h+=(std::string)"            s.insert("+enumPrefix+"EventEnum::"+ii->second[j]+");\n";
            }
        }

        buf_h+="\n\
	}\n\
        static void regEvents()\n\
        {\n";
        for(std::map<std::string, std::vector<std::string> >::iterator ii=els.begin(); ii!=els.end(); ii++)
        {
            for(size_t j=0; j<ii->second.size(); j++)
            {
                buf_h+=(std::string)"            iFactory->registerEvent("+enumPrefix+"Event::"+ii->second[j]+"::construct);\n";
            }
        }
        buf_h+="\n\
        }\n\
        protected:\n\
        static bool handle"+title+""+section+"(const REF_getter<Event::Base>& e,void *p)\n\
        {\n\
             "+title+"::Interface"+section+" *_this=("+title+"::Interface"+section+" *)p;\n\
            switch(e->id.getIid())\n\
            {\n\
";
        for(size_t j=0; j<i->second.size(); j++)
        {
            //            buf_cpp+=(std::string)"iFactory->registerEvent(EventEnum::"+i->second[j]+",Event::"+i->second[j]+"::construct, \""+i->second[j]+"\");\n";
            buf_h+=(std::string)
                   "                       case "+enumPrefix+"EventEnum::"+i->second[j]+":XTRY; return(_this->on_"+i->second[j]+"((const "+enumPrefix+"Event::"+i->second[j]+"*)e.operator->()));XPASS;\n";

        }
        buf_h+=
            "            }\n\
            return false;\n\
        }\n\
";
        for(size_t j=0; j<i->second.size(); j++)
        {
            printf("%s",i->second[j].c_str());
            buf_h+=(std::string)
                   "       virtual bool on_"+i->second[j]+"(const "+enumPrefix+"Event::"+i->second[j]+"*)=0;\n";
        }
        buf_h+=
            "    };\n\
}\n\
#endif\n\
";
        buf_cpp+=(std::string)
                 "";



        FILE *f=fopen(fn_h.c_str(),"wb");
        fputs(buf_h.c_str(),f);
        fclose(f);

    }
}



std::set<std::string> findEvents(const std::string &buf);

int main(int argc, char **argv)
{
    std::string prefix=argv[1];
    std::string total;
    for(int i=2; i<argc; i++)
    {
        total+=load_file(argv[i])+"\n";

    }

    auto vv=splitTOK("\r\n",total.c_str());
    std::string heads;
for(auto& z:vv)
{
    if(z.find("Event.h")!=std::string::npos)
    {
    printf("z %s\n",z);
    heads+=z+"\n";
    }
    
}
    std::string fn="events_"+prefix+".hpp";
    unlink(fn.c_str());
    std::set<std::string> v=findEvents(total);
    std::string getEvents,regEvents;
    getEvents+="inline std::set<EVENT_id> getEvents_"+prefix+"()\n{\n\n\tstd::set<EVENT_id> out;\n";
    regEvents+="inline void regEvents_"+prefix+"()\n{\n";
    for(auto s:v)
    {
        if(s!="InterfaceEvent::getEvents" && s!="InterfaceEvent::regEvents")
        {
            //string replace_vals(const string& regex,const string& repl, const string &src);

            std::string repl=replace_vals("Event::","EventEnum::",s);
            getEvents+="\tout.insert("+repl+");\n";
            regEvents+="\tiUtils->registerEvent("+s+"::construct);\n";
//		printf("%s\n",s.c_str());
        }
    }
    getEvents+="\n\treturn out;\n}\n";
    regEvents+="}\n";
    std::string out;
    out+="#ifndef __________"+prefix+"_EventInfo__HH\n";
    out+="#define __________"+prefix+"_EventInfo__HH\n\n\n";
    out+=heads;
    out+=getEvents+"\n";
    out+=regEvents;
    out+="#endif\n";

    FILE *f=fopen(fn.c_str(),"wb");
    fwrite(out.data(),out.size(),1,f);
    fclose(f);
    return 0;
}
