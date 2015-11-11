#include "zmq_server.hpp"

/* ZMQ includes */
#include <czmq.h>


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

static void receiver_actor(zsock_t *pipe, void *args);
/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct zmq_server_info
{
	/* add custom block local data here */
	// ZMQ subscriber
	zsock_t* subscriber;

	// actor running in separate thread to receive the messages
	zactor_t* actor;

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
	zsock_t* sub;
	/* allocate memory for the block local state */
	if ((inf = (struct zmq_server_info*)calloc(1, sizeof(struct zmq_server_info)))==NULL) {
		ERR("zmq_server: failed to alloc memory");
		ret=EOUTOFMEM;
		goto out;
	}
	b->private_data=inf;
	update_port_cache(b, &inf->ports);

	connection_spec_str = (char*) ubx_config_get_data_ptr(b, "connection_spec", &tmplen);
	printf("ZMQ connection configuration for block %s is %s\n", b->name, connection_spec_str);

	// create subscriber socket and subscribe to all messages
	sub = zsock_new_sub(connection_spec_str, "");
	zsock_set_subscribe(sub, "");

	if (!sub)
		goto out;
	// add pointer to subscriber to private data
	inf->subscriber = sub;

	ret=0;
	out:
	return ret;
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
	// clean up subscriber socket
	zsock_destroy(&inf->subscriber);
	// clean up actor thread
	zactor_destroy(&inf->actor);
	free(b->private_data);
}

/* step */
void zmq_server_step(ubx_block_t *b)
{

        //struct zmq_server_info *inf = (struct zmq_server_info*) b->private_data;

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
    int rc = zloop_reader (loop, inf->subscriber, handle_event, args);
    assert (rc == 0);
    zloop_start (loop);

    zloop_destroy (&loop);
}
