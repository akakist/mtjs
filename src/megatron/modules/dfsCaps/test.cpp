#include <stdio.h>
int main()
{
    for(int i=0; i<8; i++)
    {
        unsigned char c=1;
        c<<=7-i;
        printf("i c %d %x\n",i,c);
    }
    int n=-234;
    printf("n %d\n",n%10);
//    unsigned char c=0x40;
//    printf("%x",~c);
}