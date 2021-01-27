#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* sync bits for data word = 001b */
#define SYNC_BITS_DATA 0x1

/* sync bits for cmd word = 100b */
#define SYNC_BITS_CMD 0x4

/* sync bits for status word = 100b (which is same as cmd) */
#define SYNC_BITS_STATUS SYNC_BITS_CMD

/* sub address 5-bit max value */
#define SUB_ADDR_MAX 0x1F

/* Here we define structures for the three types of words defined
   in the MIL-STD-1553 spec: command, status, and data words
   a structure "generic word" is also defined to take in a word of
   unknown variety until it can be assigned the appropriate structure
   by looking at sync bits */

typedef struct //__attribute__((__packed__))
{
    unsigned int sync_bits:3;
    unsigned int rt_address:5;
    unsigned int tr_bit:1;
    unsigned int subaddress:5;
    unsigned int word_count:5;
    unsigned int parity_bit:1;

}command_word_s;

typedef struct //__attribute__((__packed__))
{
    unsigned int sync_bits:3;
    unsigned int rt_address:5;
    unsigned int message_error:1;
    unsigned int instrumentation:1;
    unsigned int service_request:1;
    unsigned int reserved:3;
    unsigned int brdcst_received:1;
    unsigned int busy:1;
    unsigned int subsystem_flag:1;
    unsigned int dynamic_bus_control_accept:1;
    unsigned int terminal_flag:1;
    unsigned int parity_bit:1;
}status_word_s;

typedef struct //__attribute__((__packed__))
{
    unsigned int sync_bits:3;
    unsigned int character1:8;
    unsigned int character2:8;
    unsigned int parity_bit:1;

}data_word_s;

typedef struct //__attribute__((__packed__))
{
    unsigned int sync_bits:3;
    unsigned int reserved0:5;
    unsigned int reserved1:8;
    unsigned int reserved2:4;
}generic_word_s;

char rt_memory_2d[64][2] = { {'A','A'},
                             {'A','B'},
                             {'A','C'},
                             {'A','D'},
                             {'A','E'},
                             {'A','F'},
                             {'A','H'},
                             {'A','I'},
                             {'A','J'},
                             {'A','A'}, //TODO: eloise continue here plzzz
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},
                             {'A','A'},                            
                             {'A','A'} };

/* ========== FOREWARD DECLARATIONS ========== */

void analyze_command_word(command_word_s * command_word);
void decode_data_word(data_word_s * data_word);
void build_bc_data_word(char message_byte1, char message_byte2);
void build_command_word(int rt_address, char tr_bit, int subaddress, int word_count);
void analyze_status_word(status_word_s * status_word);


// A hacky way to check that command words are created correctly :)
void print_word(command_word_s * word_check)
{
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY(word_check)  \
        (word_check->sync_bits & 0x04 ? '1' : '0'), \
        (word_check->sync_bits & 0x02 ? '1' : '0'), \
        (word_check->sync_bits & 0x01 ? '1' : '0'), \
        (word_check->rt_address & 0x10 ? '1' : '0'), \
        (word_check->rt_address & 0x08 ? '1' : '0'), \
        (word_check->rt_address & 0x04 ? '1' : '0'), \
        (word_check->rt_address & 0x02 ? '1' : '0'), \
        (word_check->rt_address & 0x01 ? '1' : '0'), \
        (word_check->tr_bit & 0x01 ? '1' : '0'), \
        (word_check->subaddress & 0x10 ? '1' : '0'), \
        (word_check->subaddress & 0x08 ? '1' : '0'), \
        (word_check->subaddress & 0x04 ? '1' : '0'), \
        (word_check->subaddress & 0x02 ? '1' : '0'), \
        (word_check->subaddress & 0x01 ? '1' : '0'), \
        (word_check->word_count & 0x10 ? '1' : '0'), \
        (word_check->word_count & 0x08 ? '1' : '0'), \
        (word_check->word_count & 0x04 ? '1' : '0'), \
        (word_check->word_count & 0x02 ? '1' : '0'), \
        (word_check->word_count & 0x01 ? '1' : '0'), \
        (word_check->parity_bit & 0x01 ? '1' : '0')

        printf("Command word: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word_check));

}

//A hacky way to check data words
void print_data_word(data_word_s * word_check)
{
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY2(word_check)  \
        (word_check->sync_bits & 0x04 ? '1' : '0'), \
        (word_check->sync_bits & 0x02 ? '1' : '0'), \
        (word_check->sync_bits & 0x01 ? '1' : '0'), \
        (word_check->character1 & 0x80 ? '1' : '0'), \
        (word_check->character1 & 0x40 ? '1' : '0'), \
        (word_check->character1 & 0x20 ? '1' : '0'), \
        (word_check->character1 & 0x10 ? '1' : '0'), \
        (word_check->character1 & 0x08 ? '1' : '0'), \
        (word_check->character1 & 0x04 ? '1' : '0'), \
        (word_check->character1 & 0x02 ? '1' : '0'), \
        (word_check->character1 & 0x01 ? '1' : '0'), \
        (word_check->character2 & 0x80 ? '1' : '0'), \
        (word_check->character2 & 0x40 ? '1' : '0'), \
        (word_check->character2 & 0x20 ? '1' : '0'), \
        (word_check->character2 & 0x10 ? '1' : '0'), \
        (word_check->character2 & 0x08 ? '1' : '0'), \
        (word_check->character2 & 0x04 ? '1' : '0'), \
        (word_check->character2 & 0x02 ? '1' : '0'), \
        (word_check->character2 & 0x01 ? '1' : '0'), \
        (word_check->parity_bit & 0x01 ? '1' : '0')

        printf("Data word: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY2(word_check));
        printf("Characters: %c%c\n", word_check->character1, word_check->character2);

}

// A hacky way to print 20 bits from any pointer
void print_void(void * void_ptr)
{
    char * word_check = (char *)void_ptr;

    #define BYTE_TO_BINARY_PATTERN3 "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY3(word_check)  \
        (word_check[0] & 0x80 ? '1' : '0'), \
        (word_check[0] & 0x40 ? '1' : '0'), \
        (word_check[0] & 0x20 ? '1' : '0'), \
        (word_check[0] & 0x10 ? '1' : '0'), \
        (word_check[0] & 0x08 ? '1' : '0'), \
        (word_check[0] & 0x04 ? '1' : '0'), \
        (word_check[0] & 0x02 ? '1' : '0'), \
        (word_check[0] & 0x01 ? '1' : '0'), \
        (word_check[1] & 0x80 ? '1' : '0'), \
        (word_check[1] & 0x40 ? '1' : '0'), \
        (word_check[1] & 0x20 ? '1' : '0'), \
        (word_check[1] & 0x10 ? '1' : '0'), \
        (word_check[1] & 0x08 ? '1' : '0'), \
        (word_check[1] & 0x04 ? '1' : '0'), \
        (word_check[1] & 0x02 ? '1' : '0'), \
        (word_check[1] & 0x01 ? '1' : '0'), \
        (word_check[2] & 0x80 ? '1' : '0'), \
        (word_check[2] & 0x40 ? '1' : '0'), \
        (word_check[2] & 0x20 ? '1' : '0'), \
        (word_check[2] & 0x10 ? '1' : '0'), \
        (word_check[2] & 0x08 ? '1' : '0'), \
        (word_check[2] & 0x04 ? '1' : '0'), \
        (word_check[2] & 0x02 ? '1' : '0'), \
        (word_check[2] & 0x01 ? '1' : '0')
        printf("Void word:    "BYTE_TO_BINARY_PATTERN3, BYTE_TO_BINARY3(word_check));

}

/* ========================================================================
   The following functions are used to create the command/data words sent 
   by the BC to the RT, which are passed to the code via the json files 
   found in the repository 
   ====================================================================== */

/* This function sends a command word for the RT to receive data and 
   then breaks the message into segments to be sent to the RT as 
   data words */
void send_data_to_rt(int rt_address, int subaddress, char message[])
{
    int message_length = strlen(message); //gets length of message so we can determine how many words to send
    int number_data_words;
    int data_word_count = 0; //used to increment number of words built
    int character_number = 0; //used to increment through characters in the message

    if((message_length & 0x1) == 0) //check if even
    {
        number_data_words = message_length/2; //we send 2 8 bit characters per word
    }
    else
    {
        number_data_words = (message_length / 2) + 1; //since we can only send 2 8bit characters per word, if there is one left over it is sent in own word
    }
    printf("Number of data words: %d\n", number_data_words);
    build_command_word(rt_address, 'R', subaddress, number_data_words); //first send command word instructing RT to receive

    for(data_word_count = 0; data_word_count < number_data_words; data_word_count++) //construct as many data words as needed to send entire message
    {
        if(character_number + 1 > message_length)
        {
            build_bc_data_word(message[character_number], ' ');
        }
        else
        {
            build_bc_data_word(message[character_number], message[character_number+1]);          
        }
        character_number += 2;

    }
    
    

}

/* This fuction sends a command word for the RT to send data*/

void request_data_from_rt(int rt_address, int subaddress, int word_count)
{
    build_command_word(rt_address, 'T', subaddress, word_count);
}

/* This fuction builds command words according the 1553 spec */

void build_command_word(int rt_address, char tr_bit, int subaddress, int word_count)
{
    command_word_s command_word;

    command_word.sync_bits = 4; //since the command word sync bits should be 100, we set this to 4 to get the correct bit values
    command_word.rt_address = rt_address;
    if (tr_bit == 'T')
        command_word.tr_bit = 1;
    else
    {
        command_word.tr_bit = 0;
    }
    command_word.subaddress = subaddress;
    command_word.word_count = word_count;
    command_word.parity_bit = 1;
    print_word(&command_word);
    //TODO send word to socket
}

/* This fuction builds data words according the 1553 spec */

void build_bc_data_word(char message_byte1, char message_byte2)
{
    data_word_s data_word;
    data_word.sync_bits = SYNC_BITS_DATA;
    data_word.character1 = (int)message_byte1;
    data_word.character2 = (int)message_byte2;
    data_word.parity_bit = 1;
    print_data_word(&data_word);
    //TODO send word to socket
}

/* The following functions are used to create the status/data words sent
   by the RTs based on the command words received */

/*void send_word_to_bc(command_word_s * command)
{

}
*/
void build_rt_data_word(int subaddress, int data_word_count)
{
    data_word_s data_word;
    int data_words_sent = 0;

    /* input validation */
    if(subaddress > SUB_ADDR_MAX)
    {
        printf("Invalid subaddress (build_rt_data_word)");
        return;
    }

    for(data_words_sent = 0; data_words_sent < data_word_count; data_words_sent++)
    {
        data_word.sync_bits = SYNC_BITS_DATA;
        data_word.character1 = (int)rt_memory_2d[subaddress+data_words_sent][0];
        data_word.character2 = (int)rt_memory_2d[subaddress+data_words_sent][1];
        data_word.parity_bit = 1;
        print_data_word(&data_word);
        //TODO: send out data_word
    }
}

void build_status_word(int rt_address)
{
    status_word_s status_word;
    status_word.sync_bits = SYNC_BITS_STATUS;
    status_word.rt_address = rt_address;
    status_word.message_error = 0;
    status_word.instrumentation = 0;
    status_word.service_request = 0;
    status_word.reserved = 0;
    status_word.brdcst_received = 0;
    status_word.busy = 0;
    status_word.subsystem_flag = 0;
    status_word.dynamic_bus_control_accept = 0;
    status_word.terminal_flag = 1;
    status_word.parity_bit = 1;
    analyze_status_word(&status_word);
}


/* The following functions are used to interpret incoming
   words and process them accordingly */

/* This function determines whether an incoming word is a
   data word or command word */

void interpret_incoming_frame_rt(generic_word_s * generic_word)
{
    if(generic_word->sync_bits == SYNC_BITS_CMD)
    {
        analyze_command_word((command_word_s *)generic_word);
    }
    else if(generic_word->sync_bits == SYNC_BITS_DATA)
    {
        decode_data_word((data_word_s *)generic_word);
    }
    else
    {
        //TODO: handle malformed packet
        //print error message
        return;
    }
    
    
}

void interpret_incoming_frame_bc(generic_word_s * generic_word)
{
    if(generic_word->sync_bits == SYNC_BITS_STATUS)
    {
        analyze_status_word((status_word_s *)generic_word);
    }
    else if(generic_word->sync_bits == SYNC_BITS_DATA)
    {
        decode_data_word((data_word_s *)generic_word);
    }
    else
    {
        //TODO: handle malformed packet
        //print error message
        return;
    }
}

void analyze_command_word(command_word_s * command_word)
{
    print_word(command_word);
    if(command_word->tr_bit == 0)
    {
        build_status_word(command_word->rt_address);
    }
    else
    {
        build_status_word(command_word->rt_address);
        build_rt_data_word(command_word->subaddress, command_word->word_count);
    }

}

void decode_data_word(data_word_s * data_word)
{
    printf("%c%c", data_word->character1, data_word->character2);
}

void analyze_status_word(status_word_s * status_word)
{
    printf("terminal_flag_bit: %d, busy bit: %d, brdcst_recvd_bit: %d, rt_address: %d, message_error_bit: %d, subsystem_flag_bit: %d,\
     dynamic_bus_control_accpt: %d, reserved_bits: %d, service_request_bit: %d, instrumentation_bit: %d", status_word->terminal_flag, status_word->busy,\
     status_word->brdcst_received, status_word->rt_address, status_word->message_error, status_word->subsystem_flag, status_word->dynamic_bus_control_accept,\
     status_word->reserved, status_word->service_request, status_word->instrumentation);
}



/* The following functions are used to initialize 
   a UDP socket */




/* ============ MAIN =========== */ 
int main()
{
    send_data_to_rt(01, 10, "Some Message");
    /*
    generic_word_s generic_word;
    generic_word.sync_bits = 4;
    generic_word.reserved0 = 1;
    generic_word.reserved1 = 128;
    generic_word.reserved2 = 15;
    print_void((void *)&generic_word);
    print_word((command_word_s *)&generic_word);
    interpret_incoming_frame_rt(&generic_word);
    */
    return 0;
}