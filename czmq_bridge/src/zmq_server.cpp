#include "zmq_server.hpp"

/* ZMQ includes */
#include <czmq.h>
#include <iostream>

UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

const unsigned int DEFAULT_BUFFER_LENGTH = 90000;

static void receiver_actor(zsock_t *pipe, void *args);
/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct zmq_server_info
{
	/* add custom block local data here */
	// ZMQ server
	zctx_t *ctx;

	void *server; //

	zsock_t* server_socket;

	// actor running in separate thread to receive the messages
	zactor_t* actor;


    // Data buffer fpr input port
    unsigned char* buffer;

    // Length of the buffer
    unsigned long buffer_length;

	/* this is to have fast access to ports for reading and writing, without
	 * needing a hash table lookup */
	struct zmq_server_port_cache ports;
};

/* init */
int zmq_server_init(ubx_block_t *b)
{
	int ret = -1;
	struct zmq_server_info *inf;
	unsigned int tmplen;
	char *connection_spec_str;

	// CZMQ socket for subscriber
	zsock_t* server_socket;
	/* allocate memory for the block local state */
	if ((inf = (struct zmq_server_info*)calloc(1, sizeof(struct zmq_server_info)))==NULL) {
		ERR("zmq_server: failed to alloc memory");
		ret=EOUTOFMEM;
		return ret;
	}
	b->private_data=inf;
	update_port_cache(b, &inf->ports);

    inf->buffer_length = DEFAULT_BUFFER_LENGTH; //TODO read from config
    inf->buffer = new unsigned char [inf->buffer_length];

	connection_spec_str = (char*) ubx_config_get_data_ptr(b, "connection_spec", &tmplen);
	printf("ZMQ connection configuration for block %s is %s\n", b->name, connection_spec_str);

    //  Create context
    inf->ctx = zctx_new ();

    //  Create and bind server socket
//    void* server = zsocket_new (inf->ctx, ZMQ_REP);
//    zsocket_bind (server, "tcp://%s:%d", "127.0.0.0", 22422);

	// create subscriber socket and subscribe to all messages
	server_socket = zsock_new_rep(connection_spec_str);
//	zsock_set_subscribe(sub, "");

	if (!server_socket) {
		return ret;
	}
	// add pointer to subscriber to private data
	inf->server_socket = server_socket;

	return 0;
}

/* start */
int zmq_server_start(ubx_block_t *b)
{
	struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;

	// incoming data is handled by the actor thread
	zactor_t* actor = zactor_new (receiver_actor, b);
	inf->actor = actor;

	int ret = 0;
	return ret;
}

/* stop */
void zmq_server_stop(ubx_block_t *b)
{
        /* struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data; */
}

/* cleanup */
void zmq_server_cleanup(ubx_block_t *b)
{
	struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;
	// clean up context socket
	zctx_destroy (&inf->ctx);
	// clean up actor thread
	zactor_destroy(&inf->actor);
	free(b->private_data);
}

/* step */
void zmq_server_step(ubx_block_t *b)
{

    struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;
//    std::cout << "zmq_server: Processing a port update" << std::endl;

	/* Read data from port */
	ubx_port_t* port = inf->ports.zmq_rep;
	assert(port != 0);

	ubx_data_t msg;
	checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
	msg.type = port->in_type;
	msg.len = inf->buffer_length;
	msg.data = inf->buffer;

//	std::cout << "zmq_server: Reading from port" << std::endl;
	int read_bytes = __port_read(port, &msg);
	if (read_bytes <= 0) {
//		std::cout << "zmq_server: No data recieved from port" << std::endl;
		return;
	}

	std::cout << "zmq_server: read bytes = " << read_bytes << std::endl;

	/* Setup ZMQ frame. At this point only single frames are sent. This can be replaced by zmsg_t messages
        if multi-part messages become necessary*/
	zframe_t* message = zframe_new(msg.data, read_bytes);
	std::cout << "Created frame of length " << zframe_size(message) << std::endl;

	/* Send the message */
	int result = zframe_send(&message, inf->server_socket,0);
	std::cout << "send message with result " << result << std::endl;

}

int handle_event(zloop_t *loop, zsock_t *reader, void *args) {
	// initialization
	ubx_block_t *b = (ubx_block_t *) args;
	struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;
	printf("zmq_server: data available.\n");

	zframe_t *frame = zframe_recv (reader);
	// print out frame data
	zframe_print (frame, NULL);

	// move to step function?
	ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
	ubx_data_t msg;
	msg.data = (void *)zframe_data(frame);
	msg.len = zframe_size(frame);
	msg.type = type;

	//hexdump((unsigned char *)msg.data, msg.len, 16);
	__port_write(inf->ports.zmq_req, &msg);

	/* Inform potential observers ? */

	// clean up temporary frame object
	zframe_destroy (&frame);

	return 1;
}

static void
receiver_actor (zsock_t *pipe, void *args)
{
    // initialization
    ubx_block_t *b = (ubx_block_t *) args;
    struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;
    printf("zmq_server: actor started.\n");
    // send signal on pipe socket to acknowledge initialisation
    zsock_signal (pipe, 0);

    zloop_t *loop = zloop_new ();
    assert (loop);
    int rc = zloop_reader (loop, inf->server_socket, handle_event, args);
    assert (rc == 0);
    zloop_start (loop);

    zloop_destroy (&loop);
}
