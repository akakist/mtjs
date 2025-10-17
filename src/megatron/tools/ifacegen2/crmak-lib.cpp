#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <sys/stat.h>
#include "regex.h"
#include <unistd.h>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"
using namespace std;
struct _rxfind_data {
    string str;
    int so;
    int eo;
};
void find(vector < _rxfind_data > &res, const char *regexp, const char *buffer);
string replace_vals(const string& regex,const string& repl, const string &src);

string join(const string &pattern, const vector<string> & arr);
vector < string > splitTOK(const char *seps, const char *src);

std::set<std::string> findEvents(const std::string &buf)
{
    std::set<std::string> out;
    {
        vector < _rxfind_data > res;
        find(res,"[A-Za-z0-9]+Event\:\:[A-Za-z0-9]+",buf.c_str());
        for(int i=0; i<res.size(); i++)
        {
            out.insert(res[i].str);

        }
    }
    {
        vector < _rxfind_data > res;
        find(res,"[A-Za-z0-9]+EventEnum\:\:[A-Za-z0-9]+",buf.c_str());
        for(int i=0; i<res.size(); i++)
        {
            std::string s=replace_vals("Enum","",res[i].str);
            out.insert(s);

        }
    }

    return out;
}

std::set<std::string> findHeadEvents(const std::string &buf)
{
    std::set<std::string> out;
    {
        vector < _rxfind_data > res;
        find(res,"[A-Za-z0-9]+\.h",buf.c_str());
        for(int i=0; i<res.size(); i++)
        {
            out.insert(res[i].str);

        }
    }
    {
        vector < _rxfind_data > res;
        find(res,"[A-Za-z0-9]+EventEnum\:\:[A-Za-z0-9]+",buf.c_str());
        for(int i=0; i<res.size(); i++)
        {
            std::string s=replace_vals("Enum","",res[i].str);
            out.insert(s);

        }
    }

    return out;
}


std::string load_file(const std::string & fn)
{

    std::string res;
#ifdef _MSC_VER
    struct _stat  st;
#else
    struct stat  st;
#endif
#ifdef _MSC_VER
    if (_stat(fn.c_str(), &st))
#else
    if (stat(fn.c_str(), &st))
#endif
    {
        fprintf(stderr,"cannot stat file %s\n",fn.c_str());
        //exit(1);
    }
    FILE *f=fopen(fn.c_str(), "rb");
    if (f)
    {
        unsigned char *b=(unsigned char *) malloc(st.st_size);
        if (b==NULL)
        {
            fprintf(stderr,"malloc error on file %s\n",fn.c_str());
            //exit(1);
        }
        if (fread(b,1,st.st_size,f)!=st.st_size)
        {
            fclose(f);
            free(b);
            fprintf(stderr,"fread error on file %s\n",fn.c_str());
            //exit(1);
        }
        std::string al((char *)b, st.st_size);
        fclose(f);
        free(b);
        res=al;
        return res;
    }
    fprintf(stderr,"call invalid error on file %s\n",fn.c_str());
    return "";
    //exit(1);
}


string join(const char* pattern, const vector<string> & arr)
{
    return join((string)pattern,arr);
}

string join(const string &pattern, const vector<string> & arr)
{
    string ret;
    if (arr.size() > 1)
    {
        unsigned int i;
        for (i = 0; i < arr.size() - 1; i++)
            ret += arr[i] + pattern;
        ret += arr[arr.size() - 1];
    }
    else if(arr.size()==1)
    {
        ret+=arr[0];
    }
    return ret;
}
bool is_file_exists(const string &pathname)
{

    struct stat st;
    bool res=false;

    if(stat(pathname.c_str(),&st))
    {   res=false;
    }
    else res=true;
    //printf("#stat %s %d\n",pathname.c_str(),stat(pathname.c_str(),&st));
//	logErr2("is_file_exists %s %d",pathname.c_str(),res);
    return res;

}
void check_path_wfn(const string & _pathname)
{
    //printf("#check_path_wfn(%s)\n",_pathname.c_str());
    string pathname=_pathname;
    if(!pathname.size())return;
    vector <string> ar=splitTOK("/",pathname.c_str());;
    vector <string> newar;
    newar.clear();
    {   for(int i=0; i<ar.size()-1; i++)
        {
            newar.push_back(ar[i]);
        }
    }

    string dir=(pathname[0]=='/'?"/":"")+join("/",newar);
//	logErr2("check dir %s",dir.c_str());

    if(is_file_exists(dir)) return;
    string parts;
    if(pathname[0]=='/')parts="/";
    for(unsigned i=0; i<newar.size(); i++)
    {
        string par=parts+newar[i];
        parts+=newar[i]+"/";
        if(/*(!is_file_exists(parts))&&*/(!is_file_exists(par)))
        {
//			logErr2("Creating dir: %s", to_mac_enc(parts).c_str());
#ifdef WIN32
            if(mkdir(parts.c_str()))
#else
            if(mkdir(parts.c_str(),0755))
#endif
            {
                printf("#Cannot create dir %s\n",parts.c_str());
                exit(0);
            }
        }
//		parts+=;
    }
}

string replace_vals(const string& regex,const string& repl, const string &src)
{
    string s;
    vector<_rxfind_data> r;
    string a;
    find(r,regex.c_str(),src.c_str());
    int lpz=0;
    for(unsigned int i=0; i<r.size(); i++) {
        a+=src.substr(lpz,r[i].so-lpz);
        a+=repl;
        lpz=r[i].eo;
    }
    a+=src.substr(lpz,src.size()-lpz);
    return a;
}
void find(vector < _rxfind_data > &res, const char *regexp, const char *buffer)
{
    int err;
    regex_t re;
    err = regcomp(&re, regexp, REG_EXTENDED);
    regmatch_t pm[10];
    memset(pm, 0, sizeof(regmatch_t));
    if (err) {
        char erbuf[111];
        regerror(err, &re, erbuf, sizeof(erbuf));
        printf ("#%s ",((string)"regex "+regexp+": "+erbuf).c_str());
        return;
    }
    int lastpos = 0;
    while (!regexec(&re, buffer + lastpos, 10, pm, 0)) {
        const char *s = buffer + lastpos;
        int lpos=lastpos;
        for (int i = 0; i < 10; i++) {
            if (pm[i].rm_so != -1) {
                int matchlen = pm[i].rm_eo - pm[i].rm_so;
                string fs(&s[pm[i].rm_so], matchlen);
                _rxfind_data fd;
                fd.str = fs;
                fd.so = pm[i].rm_so + lastpos;
                fd.eo = pm[i].rm_eo + lastpos;
                res.push_back(fd);
                lpos=pm[i].rm_eo;
            } else break;
        }
        lastpos += lpos;
    }
    regfree(&re);
}

vector < string > splitTOK(const char *seps, const char *src)
{
    vector < string > res;
    char *s = strdup((char *) src);
    char *token = strtok(s, seps);
    while (token != NULL) {
        res.push_back(token);
        token = strtok(NULL, seps);
    }
    free(s);
    return res;
}
set < string > splitTOKs(const char *seps, const char *src)
{
    set < string > res;
    char *s = strdup((char *) src);
    char *token = strtok(s, seps);
    while (token != NULL) {
        res.insert(token);
        token = strtok(NULL, seps);
    }
    free(s);
    return res;
}
static unsigned char l_table[] = {
    /*
    lowercase table
    */
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd,
    0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
    0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
    0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x90, 0x83, 0x82, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x9d, 0x9e, 0x9f, 0x90, 0x91,
    0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d,
    0x9e, 0x9f, 0xa0, 0xa2, 0xa2, 0xbc, 0xa4, 0xb4, 0xa6, 0xa7, 0xb8, 0xa9,
    0xba, 0xab, 0xac, 0xad, 0xae, 0xbf, 0xb0, 0xb1, 0xb3, 0xb3, 0xb4, 0xb5,
    0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbe, 0xbe, 0xbf, 0xe0, 0xe1,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed,
    0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
    0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
    0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd,
    0xfe, 0xff
};

static unsigned char u_table[] = {
    /*
    uppercase table
    */
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd,
    0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x41,
    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d,
    0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x81, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x80, 0x91,
    0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x8a, 0x9b, 0x8c, 0x8d,
    0x8e, 0x8f, 0xa0, 0xa1, 0xa1, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
    0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb2, 0xa5, 0xb5,
    0xb6, 0xb7, 0xa8, 0xb9, 0xaa, 0xbb, 0xa3, 0xbd, 0xbd, 0xaf, 0xc0, 0xc1,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd,
    0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
    0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
    0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd,
    0xde, 0xdf
};


std::string strupper(const std::string &s)
{

    std::string a;
    for (unsigned int i=0; i<s.length(); i++) {
        unsigned char z=s[i];
        a+=u_table[z];
    }
    return a;
}

std::string strlower(const std::string & s)
{

    /*
    convert std::string to lowercase
    */
    std::string a;
    for (unsigned int i = 0; i < s.length(); i++) {
        unsigned char z = s[i];
        a += l_table[z];
    }
    return a;

}

