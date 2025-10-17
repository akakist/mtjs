#include <vector>
#include <stdexcept>
#include <time.h>
#define N 1000
std::vector<void*> malloced;

u_int64_t last_t=0;
void ptime(const char *s)
{
    timespec tstmp;
    if(clock_gettime(CLOCK_REALTIME,&tstmp))
    {
        throw std::runtime_error("clock_gettime");
    }
    u_int64_t t=tstmp.tv_sec*1000000+tstmp.tv_nsec/1000;
    printf("%s - %lld\n",s,t-last_t);
    last_t=t;


}
void t_malloc()
{
    for(int i=0; i< N; i++)
    {
        auto p=malloc(1000000);
        malloced[i]=p;
    }
}
void t_realloc(int s)
{
    for(int i=0; i< N; i++)
    {
        auto p=realloc(malloced[i],s);
        if(!p)
            throw std::runtime_error("!p");
        malloced[i]=p;
    }
}
void t_free()
{
    for(int i=0; i< N; i++)
    {
        free(malloced[i]);
    }
}
void string_alloc()
{
    for(int i=0; i< N; i++)
    {
        auto p=new std::string();
        p->reserve(1000000);
        malloced[i]=p;
    }
}

int main()
{
    malloced.resize(N);
    ptime("start");
    t_malloc();
    ptime("t_malloc");
    t_realloc(2000000);
    ptime("t_realloc(2000000);");
    printf("realloc 1mln\n");
    t_realloc(1000000);
    ptime("t_realloc(1000000);");
    t_realloc(500000);
    ptime("t_realloc(500000);");

    t_free();
    ptime("t_free();");
    string_alloc();
    ptime("string_alloc();");
    return 0;
}
