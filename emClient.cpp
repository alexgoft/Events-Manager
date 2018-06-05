//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#include "Utils.h"

//==============================================================================
//================================= GLOBALS ====================================
//==============================================================================
// The log file
Log * client_log;

// Will be used to guard the log file
pthread_mutex_t client_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Socket to server
int sock_to_server;

// Address to server
struct sockaddr_in server;

// client name
string client_name;

// indicated whether the client is registered or not
bool is_registered = false;

char sent_message[PROTOCOL_MESSAGE_SIZE];
char received_message[PROTOCOL_MESSAGE_SIZE];

//==============================================================================
//=============================== HELPERS ======================================
//==============================================================================
/**
 * This method allows the user to send message to the server and receive its
 * response. A connection is established beforehand.
 */
void send_receive_server_comunication(char* sent_message, \
                                      char* received_message)
{
	// Create socket
	sock_to_server = socket(AF_INET, SOCK_STREAM, 0);

	int * con = new int(sock_to_server);

	if (sock_to_server < SUCCESS) {
		exit_write_close(client_log, sys_call_error("socket"), ERROR);
	}

	// Connect to server
	if (connect(*con, (struct sockaddr *) &server, sizeof(server)) < SUCCESS)
	{
		close(sock_to_server);
		exit_write_close(client_log, sys_call_error("connect"), ERROR);
	}

	// send data
	if (write_data(con, sent_message, PROTOCOL_MESSAGE_SIZE) == FAILURE)
	{
		exit_write_close(client_log, sys_call_error("write"), ERROR);
	}

	//get response
	if (read_data(con, received_message, \
	              PROTOCOL_MESSAGE_SIZE) == FAILURE)
	{
		exit_write_close(client_log, sys_call_error("read"), ERROR);
	}

	delete con;

	// close connection to server
	close(sock_to_server);
}

/**
 * Chcecks if command is legal command
 */
int is_legal_command(string command)
{
	if (command == string(REGISTER_TEXT) ||
		command == string(UNREGISTER_TEXT) ||
		command == string(CREATE_TEXT) ||
		command == string(GET_TOP_5_TEXT) ||
		command == string(SEND_RSVP_TEXT) ||
		command == string(GET_RSVPS_LIST_TEXT))
	{
		return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Client functions///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Method that used to register a client.
 */
void client_register()
{
	// create a string out of the request
	string string_to_send = client_name + STRING_DELIMITER + REGISTER_TEXT;

	// copy the string into the cahr buffer
	string_to_send.copy(sent_message, string_to_send.length());

	send_receive_server_comunication(sent_message, received_message);

	if (received_message[REQUEST_STATUS] == REQUEST_OK)
	{
		client_log->write_to_log("Client " + client_name +
								 " was registered successfully.\n");
		is_registered = true;
	}
	else
	{
		exit_write_close(client_log, "ERROR: the client " + client_name +
                         " was already registered.\n", EXIT_SUCCESS);
	}
}

/**
 * Method that used to uregister a client.
 */
void client_unregister()
{
	// create a string out of the request
	string string_to_send = client_name + STRING_DELIMITER +  UNREGISTER_TEXT;

	// copy the string into the char buffer
	string_to_send.copy(sent_message, string_to_send.length());

	send_receive_server_comunication(sent_message, received_message);

	if (received_message[REQUEST_STATUS] == REQUEST_OK)
	{
		exit_write_close(client_log, "Client " + client_name + \
                             " was unregistered successfully.\n", SUCCESS);
	}
	else
	{
		client_log->write_to_log(NOT_REGISTERED);
		is_registered = false;
	}
}

/**
 * Method that used to create an event.
 */
int client_create(vector<string> split_msg)
{
	string string_to_send;

	if (split_msg.size() < CREATE_VALID_WORD_NUM)
	{
		client_log->write_to_log("ERROR: missing arguments "
										 "in command CREATE.\n");
		return FAILURE;
	}
	else
	{
		string event_desc = get_event_description(CREATE_DESC_IDX, split_msg);

		// Check sizes of the various event attributes.
		if (split_msg[EVENT_DATE_ARG].size() > TITLE_DATE_STR_SIZE ||
			split_msg[EVENT_TITLE_ARG].size() > TITLE_DATE_STR_SIZE ||
			event_desc.size() > DESC_STR_SIZE)
		{
			client_log->write_to_log("ERROR: invalid argument "
											 "in command CREATE.\n");
			return FAILURE;
		}

		string_to_send =
				client_name + STRING_DELIMITER + CREATE_TEXT + \
                              STRING_DELIMITER + split_msg[EVENT_TITLE_ARG] + \
                              STRING_DELIMITER + split_msg[EVENT_DATE_ARG] + \
                              STRING_DELIMITER + event_desc;

	}

	string_to_send.copy(sent_message, string_to_send.length());
	send_receive_server_comunication(sent_message,
									 received_message);

	client_log->write_to_log("Event id " + \
                                 to_string(atoi(received_message)) + \
                                 " was created successfully.\n");

	return SUCCESS;
}

/**
 * Method that get the top 5.
 */
int client_get_top_5()
{
	// create a string out of the request
	string string_to_send = client_name + STRING_DELIMITER + GET_TOP_5_TEXT;

	// copy the string into the char buffer
	string_to_send.copy(sent_message, string_to_send.length());

	send_receive_server_comunication(sent_message, received_message);

	vector <string> split_events = split(received_message, \
	                                     (char) EVENT_DELIMITER);

	string events_string = EMPTY_STR;
	for (auto const &event: split_events)
	{
		events_string += event;
	}

	// incase there are no events at the moment
	events_string += ".\n";

	client_log->write_to_log(events_string);

	return SUCCESS;
}

/**
 * Method that is used to send rsvp.
 */
int client_send_rsvp(vector<string> split_msg)
{
	if(split_msg.size() == SEND__GET_RSVP_ONLY_COMMAND)
	{
		client_log->write_to_log("ERROR: missing arguments in command "
										 "SEND_RSVP_TEXT\n");
		return FAILURE;
	}

	if(!isInteger(split_msg[GET_RSVP_ID]))
	{
		client_log->write_to_log("ERROR\tclient_send_rsvp_list\t"
										 "given id is not an integer.\n");
		return FAILURE;
	}

	string string_to_send = client_name + STRING_DELIMITER + SEND_RSVP_TEXT +
							STRING_DELIMITER + split_msg[GET_RSVP_ID];

	// copy the string into the char buffer
	string_to_send.copy(sent_message, string_to_send.length());

	send_receive_server_comunication(sent_message, received_message);

	if (received_message[REQUEST_STATUS] == REQUEST_OK)
	{
		client_log->write_to_log("RSVP to event id " + split_msg[GET_RSVP_ID] +
								 " was received successfully.\n");
	}
	else if (received_message[REQUEST_STATUS] == REQUEST_OK_BUT_PLUS)
	{
		client_log->write_to_log("RSVP to event id " + split_msg[GET_RSVP_ID] +
								 " was already sent.\n");
	}
	else if (received_message[REQUEST_STATUS] == ERROR_IN_REQUEST)
	{
		client_log->write_to_log("ERROR: faild to send RSVP to event id " +
										 split_msg[GET_RSVP_ID] +
										 ": event was not found.\n");
		return FAILURE;
	}

	return SUCCESS;
}

/**
 * Method that is used to retrieve rsvp list for a given id.
 */
int client_get_rsvp_list(vector<string> split_msg)
{
	if (split_msg.size() == SEND__GET_RSVP_ONLY_COMMAND)
	{
		client_log->write_to_log("ERROR: missing arguments "
								 "in command GET_RSVPS_LIST\n");
		return FAILURE;
	}

	if(!isInteger(split_msg[GET_RSVP_ID]))
	{
		client_log->write_to_log("ERROR\tclient_get_rsvp_list\t"
										 "given id is not an integer.\n");
		return FAILURE;
	}

	// create a string out of the request
	string string_to_send = client_name + STRING_DELIMITER + \
                                GET_RSVPS_LIST_TEXT + STRING_DELIMITER + \
                                split_msg[GET_RSVP_ID];

	// copy the string into the char buffer
	string_to_send.copy(sent_message, string_to_send.length());
	send_receive_server_comunication(sent_message, received_message);

	string sorted_users = EMPTY_STR;

	vector <string> client_name_list =
			split(received_message, STRING_DELIMITER);

	std::sort(client_name_list.begin(), client_name_list.end());
	for (auto const &name: client_name_list)
		sorted_users += name + ',';


	// Event with given id doesn't exist.
	if (received_message[REQUEST_STATUS] == ERROR_IN_REQUEST) {
		client_log->write_to_log("ERROR\tclient_get_rsvp_list\tevent "
								 "with given id does not exist.\n");
		return FAILURE;
	}

	client_log->write_to_log("The RSVP's list for event id " + \
                             split_msg[GET_RSVP_ID] + " is: " + \
                             sorted_users.substr(0,sorted_users.length()-1) \
                             + ".\n");

	return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
* Parse a command and execute it
*/
int parse_command_and_execute(string msg_to_parse)
{
	// reset in_message and out_message
	memset(sent_message, END_STR, PROTOCOL_MESSAGE_SIZE);
	memset(received_message, END_STR, PROTOCOL_MESSAGE_SIZE);

	// split msg
	vector <string> split_msg = split(msg_to_parse, STRING_DELIMITER);

	// check if msg is not empty
	if (split_msg.size() > 0 &&
		is_legal_command(to_upper(split_msg[COMMAND_SPLIT_MSG_INDEX_CLIENT])))
	{
		// extract command
		string command = to_upper(split_msg[COMMAND_SPLIT_MSG_INDEX_CLIENT]);

		////////////////////////////////////////////////////////////////////////
		// REGISTER
		////////////////////////////////////////////////////////////////////////

		if (command == string(REGISTER_TEXT) && !is_registered)
		{
			client_register();
		}
		else if (command == string(REGISTER_TEXT) && is_registered)
		{
			client_log->write_to_log(ILLEGAL_COMMAND);
		}

		// check any other command, only if user is registered
		if (is_registered)
		{
			////////////////////////////////////////////////////////////////////
			// UNREGISTER
			////////////////////////////////////////////////////////////////////
			if (command == string(UNREGISTER_TEXT))
			{
				client_unregister();
			}

			////////////////////////////////////////////////////////////////////
			// CREATE
			////////////////////////////////////////////////////////////////////
			if (command == string(CREATE_TEXT))
			{
				if(client_create(split_msg) == FAILURE)
					return FAILURE;
			}

			////////////////////////////////////////////////////////////////////
			// GET_TOP_5
			////////////////////////////////////////////////////////////////////
			if (command == string(GET_TOP_5_TEXT))
			{
				if(client_get_top_5() == FAILURE)
					return FAILURE;
			}

			////////////////////////////////////////////////////////////////////
			// SEND_RSVP
			////////////////////////////////////////////////////////////////////
			if (command == string(SEND_RSVP_TEXT))
			{
				if(client_send_rsvp(split_msg) == FAILURE)
					return FAILURE;
			}

			////////////////////////////////////////////////////////////////////
			// GET_RSVPS_LIST
			////////////////////////////////////////////////////////////////////
			if (command == string(GET_RSVPS_LIST_TEXT))
			{
				if(client_get_rsvp_list(split_msg) == FAILURE)
					return FAILURE;
			}
		}
		else
		{
			client_log->write_to_log(NOT_REGISTERED);
		}
	}
	else
	{
		client_log->write_to_log(ILLEGAL_COMMAND);
	}

	return SUCCESS;
}

//==============================================================================
//================================= MAIN =======================================
//==============================================================================

int main(int argc , char *argv[])
{
	// Check usage
	if (argc != CLIENT_ARG_NUM) {
		cout << CLIENT_USAGE << endl;
		exit(SUCCESS);
	}

	// Get client name
	client_name = argv[CLIENT_ARG_NAME];

	// Create log file named <client_name>_HHMMSS.log
	client_log = new(nothrow) Log(client_name + "_" + get_current_time()+".log",
								  client_log_mutex);
	if (client_log == nullptr)
	{
		exit(ERROR);
	}

	// Open log file
	if (client_log->open_log_file() == FAILURE)
	{
		client_log->write_to_log(sys_call_error("open"));
		delete client_log;
		exit(ERROR);
	}

	// init sockaddr_in
	server = init_sockaddr(atoi(argv[CLIENT_ARG_PORT]), \
	                       inet_addr(argv[CLIENT_ARG_IP]));

	while (true)
	{
		// get user input
		string user_input;
		getline(cin, user_input);

		// parse command and execute
		if (parse_command_and_execute(user_input) == ERROR)
		{
			delete client_log;
			exit(ERROR);
		}
	}

	delete client_log;
	return SUCCESS;
}
