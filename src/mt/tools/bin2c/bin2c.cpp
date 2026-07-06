#include <stdio.h>
int main()
{
    FILE* f=fopen("/etc/mime.types","rb");
    while(1)
    {
        int c=fgetc(f);
        if(c==EOF)
            break;
        printf("%d,",c);
    }
}
