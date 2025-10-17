#include <QCoreApplication>
#include <string>
#include <set>
#include <vector>
#include <deque>

std::vector<std::string> splitString(const char *seps, const std::string & src)
{


    std::vector < std::string> res;
    std::set<char>mm;
    size_t l;
    l =::strlen(seps);
    for (unsigned int i = 0; i < l; i++)
    {
        mm.insert(seps[i]);
    }
    std::string tmp;
    l = src.size();

    for (unsigned int i = 0; i < l; i++)
    {

        if (!mm.count(src[i]))
            tmp += src[i];
        else
        {
            if (tmp.size())
            {
                res.push_back(tmp);
                tmp.clear();
            }
        }
    }

    if (tmp.size())
    {
        res.push_back(tmp);
        tmp.clear();
    }
    return res;
}
std::string join(const char* pattern,const std::deque<std::string>& arr)
{

    std::string ret;
    if (arr.size() > 1)
    {
        unsigned int i;
        for (i = 0; i < arr.size() - 1; i++)
            ret += arr[i] + pattern;
        ret += arr[arr.size() - 1];
    }
    else if (arr.size() == 1)
    {
        ret += arr[0];
    }
    return ret;

}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(argc<2)
    {
        printf("no arg");
        exit(1);
    }
    std::vector<std::string> v=splitString("-_:.",argv[1]);
    std::deque<std::string>d;
    for(auto& z:v)
    {
        d.push_back(z);
    }
    for(size_t i=0; i<d.size(); i++)
    {
        time_t n=atoll(d[i].c_str());
        if(n>100000000)
        {
            d.erase(d.begin()+i);
            struct tm t;
            localtime_r(&n,&t);
            char s[100];
            snprintf(s,sizeof (s),"%04d%02d%02d%02d%02d%02d-%s",t->tm_yday+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,argv[1]);

            printf("ne name %s\n",s);
        }
    }
    return a.exec();
}
