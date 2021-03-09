#include "MIL-STD-1553_Library.c"
#include <stdlib.h>
#include <time.h>

#define SOURCE_PORT 2000
#define DESTINATION_PORT 2001
#define RT_ADDRESS 0x00
#define USER_CLASS BC_CLASS

#define PERCENT_STATION_COMMANDS 5

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
    word_type = rand() % 2; //chooses randomly whether to send data, request data, or send a mode code

    if(word_type == 0)
    {
        send_data_to_rt(rt_address, 0, "Some Message"); //TODO future: allow for dynamic selection of subaddress and message
    }
    else if (word_type == 1)
    {

        request_data_from_rt(rt_address, RANDOM(1, 30), RANDOM(1, 62));

    }

}

int main()
{
    initialize_library(SOURCE_PORT, DESTINATION_PORT, RT_ADDRESS, USER_CLASS);

    while (1)
    {
        /* Ground station commands can be sent when the satellite
           is over the station or at a scheduled time. To simulate 
           the less predictable nature of these commands, we send a
           ground station command if a randomly generated number
           between 1 and 100 is less than/equal to 5 (so that 5% of
           bus traffic is these commands vs routine checks). This
           percentage can be modified by changing the Macro 
           PERCENT_STATION_COMMANDS */
        


        int send_ground_station_command = RANDOM(1, 100);
        if (send_ground_station_command <= PERCENT_STATION_COMMANDS)
        {
            generate_data((rand() % 2) + 1); //the rand mod 2 function serves to randomly pick which RT to send command to.
        }
        else
        {

            mode_code_e mode_code1 = RANDOM(0, 8);
            mode_code_e mode_code2 = RANDOM(10, 15);
            /* simulates routinely requesting data from flight systems */
            send_sc_command(THRUSTER, POINT_SC_AT_GROUNDSTATION);
            request_data_from_rt(THRUSTER, 1, RANDOM(1, 32));
            //request_data_from_rt(THRUSTER, 10, RANDOM(1, 32));
            //request_data_from_rt(THRUSTER, 15, RANDOM(1, 32));
            //request_data_from_rt(MULTIPLEXOR, 1, RANDOM(1, 32));
            //request_data_from_rt(MULTIPLEXOR, 5, RANDOM(1, 32));
            //request_data_from_rt(MULTIPLEXOR, 15, RANDOM(1, 32));

            //sleep(120); //After the BC requests data, we simulate dead bus time as calculations are performed

            /* Now based on the received information, the bus controller sends commands */
            send_data_to_rt(THRUSTER, 0, "Some Message");
            //send_mode_code(THRUSTER, mode_code1);
            //send_data_to_rt(MULTIPLEXOR, 0, "Some Message");
            //send_mode_code(MULTIPLEXOR, mode_code2);

            sleep(120);
        }

 
    }

    return 0;
    
}
