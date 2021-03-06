# Events Manager

This is an events management system. The system is composed of one server and multiple clients. After a client registers to the service, it can ask to receive details about new events created by other registered clients, RSVP for an event, asks to get all clients that RSVP’ed for specific event and more.

In order to do it, we've created a simple communication protocol between the server and the client. This protocol define how the server can know which command to execute (register, create a new event, etc.), which client sends the command, etc.



## Design Remarks
The design is pretty straightforward - we've maintained data structures
containing user and event object (updated using create_client() and
create_event()).

The only thing that my not be trivial, is that in order to make the server able
to wait for upcoming requests to "communicate" (looping over accept()) while
listening to the stdin (waiting for a user to type 'EXIT'), without "jamming"
the whole process, we've used threads - thread that listens to server's socket
(and process arrived requests by creating another worker thread) and a thread
that waited for the to type 'EXIT' in the server's stdin (in order to shut down
the server).
