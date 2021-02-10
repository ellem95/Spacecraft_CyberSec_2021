#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


/* sync bits for data word = 001b */
#define SYNC_BITS_DATA 0x1

/* sync bits for cmd word = 100b */
#define SYNC_BITS_CMD 0x4

/* sync bits for status word = 100b (which is same as cmd) */
#define SYNC_BITS_STATUS SYNC_BITS_CMD

/* sub address 5-bit max value */
#define SUB_ADDR_MAX 0x1F

/* Client class types */
#define BC_CLASS 1
#define RT_CLASS 2

//Defines ip address as broadcast address so all words are sent as broadcast
#define IP_ADDR "255.255.255.255"
#define BUFFER_SIZE 1024 

#ifndef BYTE_ORDER
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* Here we define structures for the three types of words defined
   in the MIL-STD-1553 spec: command, status, and data words
   a structure "generic word" is also defined to take in a word of
   unknown variety until it can be assigned the appropriate structure
   by looking at sync bits */
typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char rt_address:5;

    unsigned char tr_bit:1;
    unsigned char subaddress:5;
    unsigned char word_count1:2;

    unsigned char word_count2:3;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char rt_address:5;
    unsigned char sync_bits:3;
    
    unsigned char word_count1:2;
    unsigned char subaddress:5;
    unsigned char tr_bit:1;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char word_count2:3;   
    #endif

}command_word_s;

typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char rt_address:5;

    unsigned char message_error:1;
    unsigned char instrumentation:1;
    unsigned char service_request:1;
    unsigned char reserved:3;
    unsigned char brdcst_received:1;
    unsigned char busy:1;

    unsigned char subsystem_flag:1;
    unsigned char dynamic_bus_control_accept:1;
    unsigned char terminal_flag:1;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char rt_address:5;
    unsigned char sync_bits:3;

    unsigned char busy:1;
    unsigned char brdcst_received:1;
    unsigned char reserved:3;
    unsigned char service_request:1;
    unsigned char instrumentation:1;
    unsigned char message_error:1;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char terminal_flag:1;
    unsigned char dynamic_bus_control_accept:1;
    unsigned char subsystem_flag:1;
    #endif

}status_word_s;

typedef struct __attribute__((__packed__)) 
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char character_A1:5;

    unsigned char character_A2:3;
    unsigned char character_B1:5;

    unsigned char character_B2:3;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char character_A1:5;
    unsigned char sync_bits:3;

    unsigned char character_B1:5;
    unsigned char character_A2:3;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char character_B2:3;
    #endif

}data_word_s;

typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char reserved0:5;

    unsigned char reserved1:8;
 
    unsigned char reserved2:8;
    unsigned char padding:4;
    #else
    unsigned char reserved0:5;
    unsigned char sync_bits:3;
    
    unsigned char reserved1:8;

    unsigned char padding:4;
    unsigned char reserved2:4;
    #endif
}generic_word_s;

/* creates a structure to store unique rt/bc data passed by the simulator codes */
typedef struct
{
    unsigned int source_port;
    unsigned int destination_port;
    unsigned int rt_address;
    unsigned int user_class; //specifies whether a user is a bus controller or remote terminal
}client_cb;

client_cb control_block; //global variable for client control block

/* MACRO to split a data word character into its 2 corresponding fields in data_word_s */
#define SPLIT_CHAR(in_char, out_char_upper5, out_char_lower3) \
{                                                             \
    out_char_upper5 = ((int)in_char>>3) & 0x1F;               \
    out_char_lower3 = ((int)in_char & 0x7);                   \
}

#define COMBINE_CHAR(in_char_upper5, in_char_lower3)          \
    (in_char_upper5<<3) | (in_char_lower3)               

char rt_memory_2d[64][2] = { {'A','A'},
                             {'A','B'},
                             {'A','C'},
                             {'A','D'},
                             {'A','E'},
                             {'A','F'},
                             {'A','H'},
                             {'A','I'},
                             {'A','J'},
                             {'A','K'},
                             {'A','L'},
                             {'A','M'},
                             {'A','N'},
                             {'A','O'},
                             {'A','P'},
                             {'A','Q'},
                             {'A','R'},
                             {'A','S'},
                             {'A','T'},
                             {'A','U'},
                             {'A','V'},
                             {'A','W'},
                             {'A','X'},
                             {'A','Y'},
                             {'A','Z'},
                             {'B','A'},
                             {'B','B'},
                             {'B','C'},
                             {'B','D'},
                             {'B','E'},
                             {'B','F'},
                             {'B','G'},
                             {'B','H'},
                             {'B','I'},
                             {'B','J'},
                             {'B','K'},
                             {'B','L'},
                             {'B','M'},
                             {'B','N'},
                             {'B','O'},
                             {'B','P'},
                             {'B','Q'},
                             {'B','R'},
                             {'B','S'},
                             {'B','T'},
                             {'B','U'},
                             {'B','V'},
                             {'B','W'},
                             {'B','X'},
                             {'B','Y'},
                             {'B','Z'},
                             {'C','A'},
                             {'C','B'},
                             {'C','C'},
                             {'C','D'},
                             {'C','E'},
                             {'C','F'},
                             {'C','G'},
                             {'C','H'},
                             {'C','I'},
                             {'C','J'},
                             {'C','K'},
                             {'C','L'},                            
                             {'C','M'} };

/* ========== FOREWARD DECLARATIONS ========== */

void analyze_command_word(command_word_s * command_word);
void decode_data_word(data_word_s * data_word);
void build_bc_data_word(char message_byte1, char message_byte2);
void build_command_word(int rt_address, char tr_bit, int subaddress, int word_count);
void analyze_status_word(status_word_s * status_word);
void interpret_incoming_frame_bc(generic_word_s * generic_word);
void interpret_incoming_frame_rt(generic_word_s * generic_word);
void send_data(generic_word_s *data);


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
        (word_check->word_count1 & 0x02 ? '1' : '0'), \
        (word_check->word_count1 & 0x01 ? '1' : '0'), \
        (word_check->word_count2 & 0x04 ? '1' : '0'), \
        (word_check->word_count2 & 0x02 ? '1' : '0'), \
        (word_check->word_count2 & 0x01 ? '1' : '0'), \
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
        (word_check->character_A1 & 0x10 ? '1' : '0'), \
        (word_check->character_A1 & 0x08 ? '1' : '0'), \
        (word_check->character_A1 & 0x04 ? '1' : '0'), \
        (word_check->character_A1 & 0x02 ? '1' : '0'), \
        (word_check->character_A1 & 0x01 ? '1' : '0'), \
        (word_check->character_A2 & 0x04 ? '1' : '0'), \
        (word_check->character_A2 & 0x02 ? '1' : '0'), \
        (word_check->character_A2 & 0x01 ? '1' : '0'), \
        (word_check->character_B1 & 0x10 ? '1' : '0'), \
        (word_check->character_B1 & 0x08 ? '1' : '0'), \
        (word_check->character_B1 & 0x04 ? '1' : '0'), \
        (word_check->character_B1 & 0x02 ? '1' : '0'), \
        (word_check->character_B1 & 0x01 ? '1' : '0'), \
        (word_check->character_B2 & 0x04 ? '1' : '0'), \
        (word_check->character_B2 & 0x02 ? '1' : '0'), \
        (word_check->character_B2 & 0x01 ? '1' : '0'), \
        (word_check->parity_bit & 0x01 ? '1' : '0')

        printf("Data word: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY2(word_check));
        //printf("Characters: %c%c\n", (word_check->character_A1<<3) | (word_check->character_A2), (word_check->character_B1<<3) | (word_check->character_B2));
        printf("Characters: %c%c\n", 
                COMBINE_CHAR(word_check->character_A1, word_check->character_A2),
                COMBINE_CHAR(word_check->character_B1, word_check->character_B2));

}

// A hacky way to print 3 bytes from any pointer
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
    command_word.word_count1 = (word_count >> 3) & 0x3;
    command_word.word_count2 = (word_count) & 0x7;
    command_word.parity_bit = 1;
    //print_word(&command_word);

    //interpret_incoming_frame_rt((generic_word_s*)&command_word); //temporary for testing
    //TODO send word to socket
    send_data((generic_word_s*)&command_word);
}

/* This fuction builds data words according the 1553 spec */

void build_bc_data_word(char message_byte1, char message_byte2)
{
    data_word_s data_word;
    data_word.sync_bits = SYNC_BITS_DATA;
    SPLIT_CHAR(message_byte1, data_word.character_A1, data_word.character_A2);

    SPLIT_CHAR(message_byte2, data_word.character_B1, data_word.character_B2);
    data_word.parity_bit = 1;
    //print_data_word(&data_word);

    //interpret_incoming_frame_rt((generic_word_s*)&data_word); //temporary for testing
    //TODO send word to socket
    send_data((generic_word_s*)&data_word);
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
        //data_word.character1 = (int)rt_memory_2d[subaddress+data_words_sent][0];
        //data_word.character2 = (int)rt_memory_2d[subaddress+data_words_sent][1];
        SPLIT_CHAR((int)rt_memory_2d[subaddress+data_words_sent][0], data_word.character_A1, data_word.character_A2);
        SPLIT_CHAR((int)rt_memory_2d[subaddress+data_words_sent][1], data_word.character_B1, data_word.character_B2);
        data_word.parity_bit = 1;
        
        //interpret_incoming_frame_bc((generic_word_s*)&data_word);
        //TODO: send out data_word
        send_data((generic_word_s*)&data_word);
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

    //interpret_incoming_frame_bc((generic_word_s*)&status_word); //temporary for testing
    //analyze_status_word(&status_word); //temporary for testing
    send_data((generic_word_s*)&status_word);

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
        printf("Invalid word received from BC.");
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
        printf("Invalid word received from RT.");
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
        build_rt_data_word(command_word->subaddress, ((command_word->word_count1<<3) | command_word->word_count2));
    }

}

void decode_data_word(data_word_s * data_word)
{
    printf("%c%c\n", 
        COMBINE_CHAR(data_word->character_A1, data_word->character_A2), 
        COMBINE_CHAR(data_word->character_B1, data_word->character_B2));
}

void analyze_status_word(status_word_s * status_word)
{
    printf("Status word: terminal_flag_bit: %d, busy bit: %d, brdcst_recvd_bit: %d, rt_address: %d, message_error_bit: %d, subsystem_flag_bit: %d,\
     dynamic_bus_control_accpt: %d, reserved_bits: %d, service_request_bit: %d, instrumentation_bit: %d\n", status_word->terminal_flag, status_word->busy,\
     status_word->brdcst_received, status_word->rt_address, status_word->message_error, status_word->subsystem_flag, status_word->dynamic_bus_control_accept,\
     status_word->reserved, status_word->service_request, status_word->instrumentation);
}



/* The following functions are used to initialize 
   a UDP socket 
*/

//starts a broadcast socket to listen for incoming packets
void *initialize_listener() 
{ 
    printf("Initializing listener\n");
    int socket_listener; 
    char buffer[BUFFER_SIZE];  
    struct sockaddr_in address; 
    int n;
    unsigned int len;

    int so_broadcast = 1;

    if ( (socket_listener = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        perror("socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    }

    if(setsockopt(socket_listener, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) < 0)
    {
        printf("Error in setting broadcast option\n");
    }


    memset(&address, 0, sizeof(address)); 
       
    address.sin_family = AF_INET; 
    address.sin_port = htons(control_block.source_port); 
    address.sin_addr.s_addr = INADDR_ANY; 
      
    if ( bind(socket_listener, (const struct sockaddr *)&address, sizeof(address)) < 0 ) 
    { 
        perror("bind failed\n"); 
        exit(EXIT_FAILURE); 
    }   

    while(1)
    {
          
        n = recvfrom(socket_listener, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &address, &len); 
        buffer[n] = '\0'; 
        if (control_block.user_class == BC_CLASS)
        {
            interpret_incoming_frame_bc((generic_word_s*)buffer);
        }
        else if (control_block.user_class == RT_CLASS)
        {
            interpret_incoming_frame_rt((generic_word_s*)buffer);
        }
        else
        {
            printf("Invalid user class: %d\n", control_block.user_class);
        }
    }
    return NULL;
    
} 


//creates a socket to send data
void send_data(generic_word_s *data)
{
    //printf("Sending data\n");
    int socket_sender; 
    char buffer[BUFFER_SIZE];
    int so_broadcast;
    so_broadcast = 1; 
    struct sockaddr_in address;

    if ( (socket_sender = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        perror("socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    }

    if(setsockopt(socket_sender, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) < 0)
    {
        printf("Error in setting broadcast option\n");
    }


    memset(&address, 0, sizeof(address)); 
      
    address.sin_family = AF_INET; 
    address.sin_port = htons(control_block.destination_port); 
    address.sin_addr.s_addr = inet_addr(IP_ADDR); 

    int n, len;
    
    sendto(socket_sender, (const void *)data, sizeof(generic_word_s), 0, (const struct sockaddr *) &address, sizeof(address)); 
    //printf("message sent.\n"); 
    
}

/* Creates a control block containing all the necessary information and starts
   a listening socket */
void initialize_library(int source_port, int destination_port, int rt_address, int class)
{
    
    control_block.source_port = source_port;
    control_block.destination_port = destination_port;
    control_block.rt_address = rt_address;
    control_block.user_class = class;

    pthread_t thread1;
    int listener;
    listener = pthread_create(&thread1, NULL, initialize_listener, NULL);

}
