
#include "library.h"

void fill(int length, int* buffer,int val)
{
    for(int i = 0;i<length;i++)
    {
        buffer[i] = val;
    }
}
void reverse(int length,int * buffer)
{
    int temp = 0;
    for(int i = 0;i<(length/2);i++)
    {
        temp = buffer[i];
        buffer[i] = buffer[length-1-i];
        buffer[length-1-i] = temp;
    }
}
int Reduce(int length,int * buffer)
{
    int ret = 0;
    for(int i = 0;i<length;i++)
    {
        ret+=buffer[i];
    }
    return ret;
}
