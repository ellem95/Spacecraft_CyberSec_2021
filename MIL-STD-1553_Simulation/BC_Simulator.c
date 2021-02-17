#include "MIL-STD-1553_Library.c"
#include <stdlib.h>

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001
#define RT_ADDRESS 0x00
#define USER_CLASS BC_CLASS

/* =====================================================

   This program is used to initialize a bus controller
   on the machine simulating the BC. The port the BC is
   listening on (SOURCE_PORT) and the port the BC should
   send data to (DESTINATION_PORT) can be defined in the
   variables above. All initialization is done within
   the MIL-STD-1553_Library.

   ===================================================== */


/* Generate a random number between [min,max] (inclusive) */
#define RANDOM(min, max) \
  (min + (rand() % (max - min + 1)))


void generate_data(int rt_address)
{
    int word_type;
    word_type = rand() % 3; //chooses randomly whether to send data, request data, or send a mode code

    if(word_type == 0)
    {
        send_data_to_rt(rt_address, 10, "Some Message"); //TODO future: allow for dynamic selection of subaddress and message
    }
    else if (word_type == 1)
    {

        request_data_from_rt(rt_address, RANDOM(1, 30), RANDOM(1, 62));
    }
    else
    {
        if (rt_address == 1)
        {
            mode_code_e mode_code = (rand() % 8);
            send_mode_code(rt_address, mode_code);
        }
        else if (rt_address == 2)
        {
            mode_code_e mode_code = (rand() % 8);
            send_mode_code(rt_address, mode_code);
        }
    }
}

int main()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);

    while (1)
    {
        int action_type; //a variable to decide if the BC sends data to RT1, RT2, or sleeps
        action_type = (rand() % 3);

        if(action_type == 0) //This adds some downtime into BC/RT communications
        {
            sleep(rand() % 60);
        }
        else if(action_type == 1) //Sends or requests data from RT1
        {
            generate_data(1);
        }
        else //Sends or requests data from RT2
        {
            generate_data(2);
        }
 
    }

    return 0;
    
}
