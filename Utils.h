#ifndef EX5_UTILS_H
#define EX5_UTILS_H

//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include "Log.h"

using namespace std;

//==============================================================================
//=============================== DEFINES ======================================
//==============================================================================
#define FAILURE -1
#define ERROR 1
#define SUCCESS 0
#define ERROR_IN_REQUEST 'e'
#define REQUEST_OK 'k'
#define REQUEST_OK_BUT_PLUS 'w'
#define END_STR '\0'

#define STRING_DELIMITER ' '
// ASCII representation
#define EVENT_DELIMITER 167
#define PROTOCOL_MESSAGE_SIZE 99999

// Protocol text
#define EXIT_TEXT "EXIT"
#define REGISTER_TEXT "REGISTER"
#define UNREGISTER_TEXT "UNREGISTER"
#define CREATE_TEXT "CREATE"
#define GET_TOP_5_TEXT "GET_TOP_5"
#define SEND_RSVP_TEXT "SEND_RSVP"
#define GET_RSVPS_LIST_TEXT "GET_RSVPS_LIST"
#define EMPTY_STR ""

#define EVENT_TITLE_ARG 1
#define EVENT_DATE_ARG 2
#define TIME_STRING 9
#define GET_RSVP_ID 1

#define REQUEST_STATUS 0


//////////////////////////////// enServer //////////////////////////////////////

#define SERVER_ARG_NUM 2
#define SERVER_ARG_PORT 1
#define SERVER_USAGE "Usage: emServer portNum"
#define SERVER_LOG_FILE "emServer.log"
#define MAX_PENDING_CONNECTIONS 10
#define CLIENT_NAME_SPLIT_MSG_INDEX 0
#define COMMAND_SPLIT_MSG_INDEX_SERVER 1
#define ARG_OFFSET 1
#define DEFAULT_ID 0
#define DESC_IDX 4
#define FIVE_CLIENTS 5
#define EQUAL 0

/////////////////////////////// emClient ///////////////////////////////////////

#define CLIENT_ARG_NUM 4
#define CLIENT_ARG_NAME 1
#define CLIENT_ARG_IP 2
#define CLIENT_ARG_PORT 3
#define CLIENT_USAGE "Usage: emClient clientName serverAddress serverPort"
#define ILLEGAL_COMMAND "ERROR: illegal command.\n"
#define NOT_REGISTERED "ERROR: first command must be REGISTER.\n"
#define COMMAND_SPLIT_MSG_INDEX_CLIENT 0
#define DESC_STR_SIZE 256
#define TITLE_DATE_STR_SIZE 30
#define CREATE_VALID_WORD_NUM 4
#define CREATE_DESC_IDX 3
#define SEND__GET_RSVP_ONLY_COMMAND 1


//==============================================================================
//=============================== TYPEDEF ======================================
//==============================================================================
typedef enum {CLIENT, SERVER} Mode;


//==============================================================================
//================================ UTILS =======================================
//==============================================================================

/**
 * Check if given string is an integer.
 */
bool isInteger(const std::string & s);
/**
 * Read data from a socket. how_much bytes will be read before return.
 */
int read_data(int* sock, char *write_to_me, int how_much);

/**
 * Write data to socket.
 */
int write_data(int* sock, char* read_from_me, int how_much);
////////////////////////////////////////////////////////////////////////////////
// TCP utils

/**
 * Initialize a sockaddr_in struct
 */
sockaddr_in init_sockaddr(int port, in_addr_t addr);

////////////////////////////////////////////////////////////////////////////////
// General utils

/**
 * Get te current time in HH:MM:SS format
 */
string get_current_time();

/**
 * Get the relevant error text
 */
string sys_call_error(string function);
/**
 * A helping method that writes, in case of failures, message and exits.
 */
void exit_write_close(Log* logfile, string message, int EXIT_CODE);

////////////////////////////////////////////////////////////////////////////////
// Parser utils
/**
 * convert a string to its upper case string
 */
string to_upper(string data);
/**
 * Split a string into vector of strings using a delimiter
 */
vector<string> split(string str, char delimiter);

string get_event_description(int start_index, vector<string> split_msg);

#endif //EX5_UTILS_H
