#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include "MIL-STD-1553_Library.h"

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001


int main()
{
    intialize_socket(SOURCE_PORT);
    send_data_to_rt(01, 10, "SOME MESSAGE");
    request_data_from_rt(1, 7, 5);
    return 0;
}
