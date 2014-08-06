#include "zmq_receiver.hpp"

/* ZMQ includes */
#include <czmq.h>


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)


static void receiver_actor(zsock_t *pipe, void *args);
/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct zmq_receiver_info
{
        /* add custom block local data here */
	// ZMQ subscriber
	zsock_t* subscriber;

        // actor running in separate thread to receive the messages
	zactor_t* actor;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct zmq_receiver_port_cache ports;
};

/* init */
int zmq_receiver_init(ubx_block_t *b)
{
        int ret = -1;
        struct zmq_receiver_info *inf;
        unsigned int tmplen;
        char *connection_spec_str;
	
	// CZMQ socket for subscriber
	zsock_t* sub;
        /* allocate memory for the block local state */
        if ((inf = (struct zmq_receiver_info*)calloc(1, sizeof(struct zmq_receiver_info)))==NULL) {
                ERR("zmq_receiver: failed to alloc memory");
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
int zmq_receiver_start(ubx_block_t *b)
{
        struct zmq_receiver_info *inf = (struct zmq_receiver_info*) b->private_data;
	
	// incoming data is handled by the actor thread
	zactor_t* actor = zactor_new (receiver_actor, b);
	inf->actor = actor;

        int ret = 0;
        return ret;
}

/* stop */
void zmq_receiver_stop(ubx_block_t *b)
{
        /* struct zmq_receiver_info *inf = (struct zmq_receiver_info*) b->private_data; */
}

/* cleanup */
void zmq_receiver_cleanup(ubx_block_t *b)
{
	struct zmq_receiver_info *inf = (struct zmq_receiver_info*) b->private_data;
	// clean up subscriber socket
	zsock_destroy(&inf->subscriber);
	// clean up actor thread
	zactor_destroy(&inf->actor);
        free(b->private_data);
}

/* step */
void zmq_receiver_step(ubx_block_t *b)
{

        //struct zmq_receiver_info *inf = (struct zmq_receiver_info*) b->private_data;

}

static void
receiver_actor (zsock_t *pipe, void *args)
{
    // initialization
    ubx_block_t *b = (ubx_block_t *) args;
    struct zmq_receiver_info *inf = (struct zmq_receiver_info*) b->private_data;
    printf("zmq_receiver: actor started.\n");
    // send signal on pipe socket to acknowledge initialisation
    zsock_signal (pipe, 0);

    bool terminated = false;
    while (!terminated) {
        //zmsg_t *msg = zmsg_recv (pipe);
        //if (!msg)
        //    break;              //  Interrupted
        //char *command = zmsg_popstr (msg);
        //if (streq (command, "$TERM"))
        //    terminated = true;
        //free (command);
        //zmsg_destroy (&msg);

        // try to receive frame 
        zframe_t *frame = zframe_recv (inf->subscriber);
        // print out frame data
        zframe_print (frame, NULL);

        // move to step function?
        ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
        ubx_data_t msg;
        msg.data = (void *)zframe_data(frame);
        msg.len = zframe_size(frame);
        msg.type = type;

        //hexdump((unsigned char *)msg.data, msg.len, 16);
        __port_write(inf->ports.zmq_in, &msg);

        /* Inform potential observers ? */

        // clean up temporary frame object
        zframe_destroy (&frame);
    }
}
