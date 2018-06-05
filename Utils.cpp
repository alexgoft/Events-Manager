#ifndef EX5_UTILS_CPP
#define EX5_UTILS_CPP

//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#include "Utils.h"

using namespace std;

//==============================================================================
//================================ UTILS =======================================
//==============================================================================
/**
 * Check if given string is an integer.
 */
bool isInteger(const std::string & s)
{
    if(s.empty() || ((!isdigit(s[0])) && \
    (s[0] != '-') && (s[0] != '+'))) return false ;

    char * p ;
    strtol(s.c_str(), &p, 10) ;

    return (*p == 0) ;
}

/**
 * Read data from a socket. how_much bytes will be read before return.
 */
int read_data(int *sock, char *write_to_me, int how_much)
{
    int b_count = 0;
    int bytes_read = 0;

    // loop until full buffer
    while (b_count < how_much)
    {
        bytes_read = (int) read(*sock, write_to_me, \
                                (size_t) how_much - b_count);

        // if there is more to read, read
        if (bytes_read > 0)
        {
            b_count += bytes_read;
            write_to_me += bytes_read;
            continue;
        }

        // if read was failed, fail as well
        if (bytes_read == FAILURE)
        {
            return FAILURE;
        }
    }

    return b_count;
}

/**
 * Write data to socket.
 */
int write_data(int *sock, char* read_from_me, int how_much) {
    int bytes_written = 0;  /* counts bytes written returned */
    int b_count = 0;    /* count how much written */

    /* loop until full buffer */
    while (b_count < how_much) {
        bytes_written = (int) write(*sock, \
                                    read_from_me, (size_t) how_much - b_count);

        // if there is more to write, write
        if (bytes_written > 0) {

            b_count += bytes_written;
            read_from_me += bytes_written;
        }

        // if write was failed, fail as well
        if (bytes_written < 0) {
            return FAILURE;
        }

        // stop writing
        break;
    }
    return b_count;
}
////////////////////////////////////////////////////////////////////////////////
// TCP utils
/**
 * Initialize a sockaddr_in struct
 */
sockaddr_in init_sockaddr(int port, in_addr_t addr)
{
    struct sockaddr_in server_sock;

    server_sock.sin_family = AF_INET;
    server_sock.sin_port = htons((uint16_t) port);
    server_sock.sin_addr.s_addr = addr;

    return server_sock;
}


////////////////////////////////////////////////////////////////////////////////
// General utils
/**
 * Get te current time in HH:MM:SS format
 */
string get_current_time()
{
    time_t current_time;
    struct tm * time_info;
    char timeString[TIME_STRING];  // space for "HH:MM:SS\0"

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
    return string(timeString);
}

/**
 * Get the relevant error text. On client, also print error to stdout
 */
string sys_call_error(string function)
{
    string error_text = "ERROR\t" + function + "\t" + to_string(errno) + ".\n";
    return error_text;
}

/**
 * Simple method that allows, in case of an error, to write message to the log
 * file and close it.
 */
void exit_write_close(Log* logfile, string message, int EXIT_CODE)
{
    logfile->write_to_log(message);
    delete logfile;
    exit(EXIT_CODE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Parser utils
/**
 * convert a string to its upper case string
 */
string to_upper(string data){
    transform(data.begin(), data.end(), data.begin(), ::toupper);
    return data;
}

/**
 * Split a string into vector of strings using a delimiter
 */
vector<string> split(string str, char delimiter) {
	vector<string> internal;
	stringstream ss(str); // Turn the string into a stream.
	string tok;

	while (getline(ss, tok, delimiter)) {
		internal.push_back(tok);
	}

	return internal;
}

/**
 * Given a starting index (3 for client, 4 for server), the event description
 * will be processed from split_msg.
 */
string get_event_description(int start_index, vector<string> split_msg)
{
    string event_desc = "";

    // CLIENT SIDE 0 - function, 1 - title, 2 - date, 3 till end - desc
    // SERVER SIDE 0 name , 1 - function, 2 - title, 3 - date, 4 till end - desc
    for (int i = start_index ; i < (int)split_msg.size() ; i++) {
        if ((unsigned)i == (split_msg.size() - 1 )) {
            event_desc += split_msg[i];
        } else {
            event_desc += split_msg[i] + STRING_DELIMITER;
        }
    }

    return event_desc;
}

#endif //EX5_UTILS_CPP