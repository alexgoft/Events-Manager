//==============================================================================
//=============================== INCLUDES  ====================================
//==============================================================================
#include <thread_db.h>
#include "Utils.h"

//==============================================================================
//=============================== STRUCTS ======================================
//==============================================================================
typedef struct
{
	string name;
	int event_id_RSVP;
} Client;

typedef struct
{
	string event_title;
	string event_date;
	string event_description;
	int event_id;
	vector<Client*> RSVP_list;
} Event;

//==============================================================================
//=============================== TYPEDEG ======================================
//==============================================================================
typedef std::vector<Client*>::iterator ClientsIter;

//==============================================================================
//============================ DATA STRUCTURES =================================
//==============================================================================
vector<Event*> events;
vector<Client*> registered_clients;

//==============================================================================
//================================= GLOBALS ====================================
//==============================================================================
// The log file
Log * server_log;

// Will be used to guard the log file
pthread_mutex_t server_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Will be used to guard the log file
pthread_mutex_t avaliable_id_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t events_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t threads_num_mutex = PTHREAD_MUTEX_INITIALIZER;
int threads_num = 0;

// this thread listens to the stdin
pthread_t stdin_thread;

// this thread listens to the clients sockets
pthread_t clients_thread;

// the server socket
int server_sock;

// the last used id
int avaliable_id = 1;

bool toExit = false;

//==============================================================================
//=============================== HELPERS ======================================
//==============================================================================

/**
 * free allocated memory to all dynamiclly allocated memories.
 * */
void free_allocated_memory()
{
	for(auto& client : registered_clients)
	{
		delete client;
	}

	for(auto& event : events)
	{
		delete event;
	}
}

/**
 * This function establishes a connection for a given sockaddr_in
 */
int establish_connection(sockaddr_in& connection_info)
{
    int client;

    // Create socket
    if ((client = socket(AF_INET, SOCK_STREAM, 0)) < SUCCESS)
	{
        server_log->write_to_log(sys_call_error("socket"));
        return FAILURE;
    }

    // Bind `sock` with given addresses
    if (bind(client, (struct sockaddr *) &connection_info, \
             sizeof(struct sockaddr_in)) < SUCCESS)
	{
        close(client);
        server_log->write_to_log(sys_call_error("bind"));
        return FAILURE;
    }

    // Max # of queued connects
    if (listen(client, MAX_PENDING_CONNECTIONS) < SUCCESS)
	{
        server_log->write_to_log(sys_call_error("listen"));
        return FAILURE;
    }

    return client;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Get the first connection request on the queue of pending connections for the
 * listening socket
 */
int get_ready_connection(int s)
{
    /* socket of connection */
    int caller;
    if ((caller = accept(s,NULL,NULL)) < SUCCESS)
	{
        server_log->write_to_log(sys_call_error("accept"));
        return FAILURE;
    }

    return caller;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * Given client is deleted, and if neccessary, removed from an event he's
 * RSVP.
 */
void delete_client(string client_name, std::vector<Client*>::iterator it)
{
	// Remove from RSVP if needed.
	if((*it)->event_id_RSVP != SUCCESS)
	{
		pthread_mutex_lock(&events_mutex);

		for(auto const& event: events)
		{
			if(event->event_id == (*it)->event_id_RSVP)
			{
				for(ClientsIter rsvp_client = event->RSVP_list.begin(); \
					rsvp_client != event->RSVP_list.end(); ++rsvp_client)
				{
					if ((*it)->name == (*rsvp_client)->name)
					{
						event->RSVP_list.erase(rsvp_client);
						break;
					}
				}

				break;
			}
		}
		pthread_mutex_unlock(&events_mutex);
	}

	delete *it; // Deallocate resources for client
	registered_clients.erase(it);

	server_log->write_to_log(
			client_name + "\twas unregistered successfully.\n");
}

/**
 * Checks is client_name is already registered. if to_del is true, client is
 * also deleted (default is false)
 */
bool is_client_registered(string client_name, bool to_del = false)
{
	// Search among registered clients.
	pthread_mutex_lock(&clients_mutex);
	for(std::vector<Client*>::iterator it = registered_clients.begin(); \
	    it != registered_clients.end(); ++it)
	{
		if (to_upper((*it)->name) == to_upper(client_name))
		{
			// delete it if needed
			if (to_del)
			{
				delete_client(client_name, it);
			}
			pthread_mutex_unlock(&clients_mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
	return false;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Initialize resources for a new client.
 */
void create_client(string client_name)
{
	Client* new_client = new Client;
	new_client->name = client_name;
	new_client->event_id_RSVP = DEFAULT_ID;

	pthread_mutex_lock(&clients_mutex);
	registered_clients.push_back(new_client);
	pthread_mutex_unlock(&clients_mutex);

	server_log->write_to_log(new_client->name + \
	                         "\twas registered successfully.\n");
}

/**
 * Initialize resources for a new event.
 * return the newly created event's id.
 */
int create_event(string client_name, vector<string> split_msg)
{
	Event* new_event = new Event;

	// Offset of one since we've sent, in addition, client's name.
	new_event->event_title = split_msg[EVENT_TITLE_ARG + ARG_OFFSET];
	new_event->event_date = split_msg[EVENT_DATE_ARG + ARG_OFFSET];
	new_event->event_description = get_event_description(DESC_IDX, split_msg);

	// Get unique id
	pthread_mutex_lock(&avaliable_id_mutex);
	new_event->event_id = avaliable_id;
	avaliable_id++;
	pthread_mutex_unlock(&avaliable_id_mutex);

	pthread_mutex_lock(&events_mutex);
	events.push_back(new_event);
	pthread_mutex_unlock(&events_mutex);


	server_log->write_to_log(client_name + "\tevent id " + \
	                         to_string(new_event->event_id) + \
							 " was assigned to the event with title " + \
							 new_event->event_title + ".\n");

	return new_event->event_id;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Assuming given client is registered, his name will be search across all over
 * registered clients list, and when found, pointer to this clients instance
 * will be returned.
 */
Client* retrieve_client_by_name(string client_name)
{
	Client* found_client = nullptr;

	pthread_mutex_lock(&clients_mutex);
	for(auto const& client: registered_clients)
	{
		if(client->name == client_name)
		{
			found_client = client;
		}
	}
	pthread_mutex_unlock(&clients_mutex);

	return found_client;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Parse a command and execute it.
 */
int parse_command_and_execute(int *current_connection, string msg_to_parse)
{
	// reset out_message
	char out_message[PROTOCOL_MESSAGE_SIZE];
	memset(out_message, '\0', PROTOCOL_MESSAGE_SIZE);

	// split msg
	vector<string> split_msg = split(msg_to_parse, STRING_DELIMITER);

	// extract name and command
	string client_name = split_msg[CLIENT_NAME_SPLIT_MSG_INDEX];
	string command = split_msg[COMMAND_SPLIT_MSG_INDEX_SERVER];

	////////////////////////////////////////////////////////////////////////////
	// REGISTER
	////////////////////////////////////////////////////////////////////////////
	if (command == string(REGISTER_TEXT))
	{
		if (is_client_registered(client_name))
		{
			// client already registered
			out_message[REQUEST_STATUS] = ERROR_IN_REQUEST;
			server_log->write_to_log(
					"ERROR: " + client_name + "\tis already exists.\n");
		}
		else
		{
			// registered successfully
			out_message[REQUEST_STATUS] = REQUEST_OK;
			create_client(client_name);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// UNREGISTER
	////////////////////////////////////////////////////////////////////////////
	if (command == string(UNREGISTER_TEXT))
	{
		if (is_client_registered(client_name, true))
		{
			out_message[REQUEST_STATUS] = REQUEST_OK;
		}
		else
		{
			// Given client does't exist among registered users.
			out_message[REQUEST_STATUS] = ERROR_IN_REQUEST;
			server_log->write_to_log(
					"ERROR: " + client_name + "\tdoes not exist.\n");
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// CREATE
	////////////////////////////////////////////////////////////////////////////
	if (command == string(CREATE_TEXT))
	{
		int new_id = create_event(client_name, split_msg);

		// return to client the corresponding string
		string new_id_str = std::to_string(new_id);
		new_id_str.copy(out_message, new_id_str.length());
	}

	////////////////////////////////////////////////////////////////////////////
	// GET_TOP_5
	////////////////////////////////////////////////////////////////////////////
	if(command == string(GET_TOP_5_TEXT))
	{
		server_log->write_to_log(
				client_name + "\trequests the top 5 newest events.\n");

		string return_message = "Top 5 newest events are:\n";
		string event_representation = EMPTY_STR;
		int counter = 0;

		pthread_mutex_lock(&events_mutex);
		// Iterate from the end to
		for (vector<Event*>::reverse_iterator it = events.rbegin();
			 it != events.rend(); ++it )
		{
			if(counter < FIVE_CLIENTS)
			{
				event_representation = to_string((*it)->event_id) + "\t" +\
									   (*it)->event_title + "\t" +\
									   (*it)->event_date + "\t" +\
									   (*it)->event_description + ".\n";
				return_message += event_representation + (char) EVENT_DELIMITER;
				counter++;
			}
			else
			{
				break;
			}
		}
		// Incase the event data structure is empty.
		if (events.size() == 0)
			return_message += '\n' + (char) EVENT_DELIMITER;
		pthread_mutex_unlock(&events_mutex);

		// -1 in order to not include the delimiter in the final representation
		return_message.copy(out_message, return_message.length() - 1);
	}

	////////////////////////////////////////////////////////////////////////////
	// SEND_RSVP
	////////////////////////////////////////////////////////////////////////////
	if (command == string(SEND_RSVP_TEXT))
	{
		int id_to_RSVP = stoi(split_msg[GET_RSVP_ID + ARG_OFFSET]);

		Client* client_in_list = nullptr;

		bool found_event = false;
		bool found_in_RSVP = false;

		pthread_mutex_lock(&events_mutex);
		for(auto const& event: events)
		{
			if (event->event_id == id_to_RSVP)
			{
				found_event = true;

				// Check if client is already RSVP.
				for(auto const& client_in_RSVP: event->RSVP_list)
				{
					// Check if, for given id's RSVP list, client exists.
					if (client_in_RSVP->name == client_name)
					{
						found_in_RSVP = true;
						out_message[REQUEST_STATUS] = REQUEST_OK_BUT_PLUS;
						break;
					}
				}

				// Went through all clients and didn't find
				if (!found_in_RSVP)
				{
					client_in_list = retrieve_client_by_name(client_name);
					event->RSVP_list.push_back(client_in_list);
					client_in_list->event_id_RSVP = id_to_RSVP;

					out_message[REQUEST_STATUS] = REQUEST_OK;

					break;
				}
			}
		}
		pthread_mutex_unlock(&events_mutex);

		// Event for given id was not found.
		if (!found_event)
		{
			out_message[REQUEST_STATUS] = ERROR_IN_REQUEST;
			server_log->write_to_log("ERROR\tparse_command_and_execute\tevent "
											 "with given id does not exist.\n");
		}
		else
		{
			server_log->write_to_log(client_name + "\tis RSVP "
									 "to event with id " +
									 split_msg[GET_RSVP_ID + \
									 ARG_OFFSET] + ".\n");
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// GET_RSVPS_LIST
	////////////////////////////////////////////////////////////////////////////
	if (command == string(GET_RSVPS_LIST_TEXT))
	{
		int id_to_RSVP_list = stoi(split_msg[GET_RSVP_ID + 1]);

		vector<string> client_name_list;

		string ready_name_list = EMPTY_STR;

		bool found_event = false;

		pthread_mutex_lock(&events_mutex);
		for (auto const &event: events)
		{
			if (event->event_id == id_to_RSVP_list)
			{
				found_event = true;

				// Check if client is already RSVP.
				for (auto const &client_in_RSVP: event->RSVP_list)
				{
					client_name_list.push_back(client_in_RSVP->name);
				}

				for (auto const &name: client_name_list)
					ready_name_list += name + ' ';

				break;
			}

		}
		pthread_mutex_unlock(&events_mutex);

		// Event for given id was not found.
		if (!found_event)
		{
			out_message[REQUEST_STATUS] = ERROR_IN_REQUEST;
			server_log->write_to_log("ERROR\tparse_command_and_execute\tevent "
											 "with given id does not exist.\n");
		}
		else
		{
			// -1 in order to not include the delimiter in the final represent'.
			ready_name_list.copy(out_message, ready_name_list.length() - 1);
			server_log->write_to_log(
					client_name + "\trequests the RSVP’s list for event " \
                    "with id " + split_msg[GET_RSVP_ID + 1] + ".\n");
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// Send back information and clean up
	////////////////////////////////////////////////////////////////////////////
	if (write_data(current_connection, out_message,
				   PROTOCOL_MESSAGE_SIZE) == FAILURE)
	{
		server_log->write_to_log(sys_call_error("write"));
		exit(ERROR);
	}

	// close that connection
	close(*current_connection);

	return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * This function listens to the stdin and signals to the server to shut down
 * when "exit" is typed.
 */
void * stdin_thread_func(void * /*args*/)
{
    // loop until exit is entered
    while (!toExit)
	{
		// get user input
		string user_input;
		getline(cin, user_input);

		if ((strcasecmp(user_input.c_str(), EXIT_TEXT) == EQUAL))
		{
            toExit = true;
        }
    }

	pthread_exit(SUCCESS);
}

/**
 * This function performes a single command
 */
void * command_thread_func(void * args)
{
	// get connection from args
	int* current_conn = (int *) args;

	// reset in_message
	char in_message[PROTOCOL_MESSAGE_SIZE];
	memset(in_message, '\0', PROTOCOL_MESSAGE_SIZE);

	// read data from the client
	if (read_data(current_conn, in_message, \
	              PROTOCOL_MESSAGE_SIZE) == FAILURE)
	{
		free_allocated_memory();
		exit_write_close(server_log, sys_call_error("read"), ERROR);
	}

	// parse command and execute
	if (parse_command_and_execute(current_conn, string(in_message)) == ERROR)
	{
		free_allocated_memory();

		delete server_log;
		exit(ERROR);
	}

	delete current_conn;
	pthread_exit(SUCCESS);
}

/**
 * This function listens to the opened socket, and proccess new requests
 */
void * clients_thread_func(void * /*args*/)
{
	pthread_mutex_lock(&threads_num_mutex);
	threads_num++;
	pthread_mutex_unlock(&threads_num_mutex);

	while (true)
	{
		// get the first connection request
		int current_connection = get_ready_connection(server_sock);
		if (current_connection == FAILURE)
		{
			free_allocated_memory();
			exit_write_close(server_log, \
				sys_call_error("accept"), ERROR);
		}

		// if exit was not typed by the server's stdin, process the request
		if (!toExit)
		{
			int * con = new int(current_connection);
			//*con = current_connection;
			pthread_t thread;

			// create thread
			if (pthread_create(&thread, NULL, \
			                   command_thread_func, con) != SUCCESS)
			{
				free_allocated_memory();
				exit_write_close(server_log, \
				sys_call_error("pthread_create"), ERROR);
			}

			pthread_detach(thread);
		}
		else
		{
			pthread_mutex_lock(&threads_num_mutex);
			threads_num--;
			pthread_mutex_unlock(&threads_num_mutex);

			pthread_exit(SUCCESS);
		}
	}
}

//==============================================================================
//================================= MAIN =======================================
//==============================================================================

int main(int argc , char *argv[])
{
	// Check usage
	if (argc != SERVER_ARG_NUM)
	{
		cout << SERVER_USAGE << endl;
		exit(SUCCESS);
	}

	// Create log file
	server_log = new(nothrow) Log(SERVER_LOG_FILE, server_log_mutex);
	if (server_log == nullptr)
	{
		exit_write_close(server_log, sys_call_error("new"), ERROR);
	}

	// Open log file
	if (server_log->open_log_file() == FAILURE)
	{
		delete server_log;
		exit(ERROR);
	}

	// init sockaddr_in
	struct sockaddr_in server = init_sockaddr(atoi(argv[SERVER_ARG_PORT]),
											  INADDR_ANY);
	// Establish connection
	server_sock = establish_connection(server);
	if (server_sock == FAILURE)
	{
		delete server_log;
		exit(ERROR);
	}

	// Create stdin thread - this tread will listen to the stdin for "ERROR" txt
	if (pthread_create(&stdin_thread, NULL, stdin_thread_func, NULL) != SUCCESS)
	{
		exit_write_close(server_log, \
		                 sys_call_error("pthread_create"), ERROR);
	}

	// Create clients thread - this tread will listen to opened sockets
	if (pthread_create(&clients_thread, NULL, \
	                   clients_thread_func, NULL) != SUCCESS)
	{
		exit_write_close(server_log, \
		                 sys_call_error("pthread_create") ,ERROR);
	}

	// wait for exit input from user
	int ret_join = pthread_join(stdin_thread, nullptr);
	if (ret_join != SUCCESS)
	{
		free_allocated_memory();
		exit_write_close(server_log, \
		                 sys_call_error("pthread_join"), ERROR);
	}

	server_log->write_to_log("EXIT command is typed: server is shutdown.\n");

	while(!toExit && threads_num > 0);

	server_log->close_log_file();

	free_allocated_memory();
	close(server_sock);
	delete server_log;

	return SUCCESS;
}