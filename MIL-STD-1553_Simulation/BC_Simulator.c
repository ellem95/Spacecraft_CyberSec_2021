#include "MIL-STD-1553_Library.c"

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001
#define RT_ADDRESS 0x00
#define USER_CLASS 1


int main()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);
    send_data_to_rt(01, 10, "SOME MESSAGE");
    request_data_from_rt(1, 7, 5);
    while (1);
    return 0;
    
}
