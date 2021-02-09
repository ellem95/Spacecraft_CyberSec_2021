#include "MIL-STD-1553_Library.c"

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001
#define RT_ADDRESS 0x01
#define USER_CLASS RT_CLASS


int main()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);
    while (1);
    return 0;
    
}
