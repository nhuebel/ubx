#include "zmq_sender.hpp"

/* Generic includes */
#include <iostream>

/* ZMQ includes */
#include <czmq.h>

UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

const unsigned int DEFAULT_BUFFER_LENGTH = 20000;

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct zmq_sender_info
{
        /* add custom block local data here */

        // ZMQ publisher
        zsock_t* publisher;

        // Data buffer fpr input port
        unsigned char* buffer;

        // Length of the buffer
        unsigned long buffer_length;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct zmq_sender_port_cache ports;
};

/* init */
int zmq_sender_init(ubx_block_t *b)
{
        int ret = -1;
        struct zmq_sender_info *inf;
        unsigned int tmplen;
        char *connection_spec_str;
        std::string connection_spec;

	// czmq socket for publisher
	zsock_t* pub;

        /* allocate memory for the block local state */
        if ((inf = (struct zmq_sender_info*)calloc(1, sizeof(struct zmq_sender_info)))==NULL) {
                ERR("zmq_sender: failed to alloc memory");
                ret=EOUTOFMEM;
                goto out;
        }

	// add the zmq_sender_info struct to private data to be able to access it in start, step and stop. alternatively (and better) this can be accomplished using an internal iblock (see https://github.com/ejans/internal_iblock_example)
        b->private_data=inf;
        update_port_cache(b, &inf->ports);

        inf->buffer_length = DEFAULT_BUFFER_LENGTH; //TODO read from config
        inf->buffer = new unsigned char [inf->buffer_length];

        connection_spec_str = (char*) ubx_config_get_data_ptr(b, "connection_spec", &tmplen);
	printf("ZMQ connection configuration for block %s is %s\n", b->name, connection_spec_str);
	
	// creating the publisher based on the connection configuration, e.g. tcp://*:11411 or ipc://data
	pub = zsock_new_pub(connection_spec_str);
	if(!pub)
		goto out;
	// add a pointer to the publisher to the private data
	inf->publisher=pub;

        ret=0;
out:
        return ret;
}

/* start */
int zmq_sender_start(ubx_block_t *b)
{
        /* struct zmq_sender_info *inf = (struct zmq_sender_info*) b->private_data; */
        int ret = 0;
        return ret;
}

/* stop */
void zmq_sender_stop(ubx_block_t *b)
{
        /* struct zmq_sender_info *inf = (struct zmq_sender_info*) b->private_data; */
}

/* cleanup */
void zmq_sender_cleanup(ubx_block_t *b)
{
        struct zmq_sender_info *inf = (struct zmq_sender_info*) b->private_data;
        delete inf->buffer;
        // clean up the publisher socket
	zsock_destroy(&inf->publisher);
        free(b->private_data);
}

/* step */
void zmq_sender_step(ubx_block_t *b)
{

        struct zmq_sender_info *inf = (struct zmq_sender_info*) b->private_data;
//        std::cout << "zmq_sender: Processing a port update" << std::endl;

	/* Read data from port */
	ubx_port_t* port = inf->ports.zmq_out;
	assert(port != 0);

	ubx_data_t msg;
	checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
	msg.type = port->in_type;
	msg.len = inf->buffer_length;
	msg.data = inf->buffer;

//	std::cout << "zmq_sender: Reading from port" << std::endl;
	int read_bytes = __port_read(port, &msg);
	if (read_bytes <= 0) {
//		std::cout << "zmq_sender: No data recieved from port" << std::endl;
		return;
	}

	std::cout << "zmq_sender: read bytes = " << read_bytes << std::endl;

	/* Setup ZMQ frame. At this point only single frames are sent. This can be replaced by zmsg_t messages 
           if multi-part messages become necessary*/
	zframe_t* message = zframe_new(msg.data, read_bytes);
	std::cout << "Created frame of length " << zframe_size(message) << std::endl;

	/* Send the message */
	int result = zframe_send(&message, inf->publisher,0);
	std::cout << "send message with result " << result << std::endl;

}

