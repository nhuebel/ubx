#include "sherpa_msg_bridge.hpp"

#include <iostream>

UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

const unsigned int DEFAULT_BUFFER_LENGTH = 20000;

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct sherpa_msg_info
{
  int bridge_direction; // 1 = send; 2 = receive	

  // Data buffer fpr input port
  unsigned char* buffer;

  // Length of the buffer
  unsigned long buffer_length;

	/* this is to have fast access to ports for reading and writing, without
	 * needing a hash table lookup */
	struct sherpa_msg_port_cache ports;
};

/* init */
int sherpa_msg_init(ubx_block_t *b)
{
	int ret = -1;
	struct sherpa_msg_info *inf;
//	unsigned int tmplen;


  /* allocate memory for the block local state */
  if ((inf = (struct sherpa_msg_info*)calloc(1, sizeof(struct sherpa_msg_info)))==NULL) {
          ERR("sherpa_msg: failed to alloc memory");
          ret=EOUTOFMEM;
          goto out;
  }

	b->private_data=inf;
	update_port_cache(b, &inf->ports);


  inf->buffer_length = DEFAULT_BUFFER_LENGTH; //TODO read from config
  inf->buffer = new unsigned char [inf->buffer_length];

  /* Parameters. E.g. if the breadge reads or writes */

	//connection_spec_str = (char*) ubx_config_get_data_ptr(b, "connection_spec", &tmplen);
	//printf("ZMQ connection configuration for block %s is %s\n", b->name, connection_spec_str);

  ret=0;
out:
  return ret;
}

/* start */
int sherpa_msg_start(ubx_block_t *b)
{
	//struct sherpa_msg_info *inf = (struct sherpa_msg_info*) b->private_data;
	return 0;
}

/* stop */
void sherpa_msg_stop(ubx_block_t *b)
{
  /* struct sherpa_msg_info *inf = (struct sherpa_msg_info*) b->private_data; */
}

/* cleanup */
void sherpa_msg_cleanup(ubx_block_t *b)
{
	struct sherpa_msg_info *inf = (struct sherpa_msg_info*) b->private_data;
  delete inf->buffer;
	free(b->private_data);
}

/* step */
void sherpa_msg_step(ubx_block_t *b)
{

    struct sherpa_msg_info *inf = (struct sherpa_msg_info*) b->private_data;
//    std::cout << "sherpa_msg: Processing a port update" << std::endl;

	/* Read data from port */
	ubx_port_t* port = inf->ports.msg_in;
	assert(port != 0);

	ubx_data_t msg;
	checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
	msg.type = port->in_type;
	msg.len = inf->buffer_length;
	msg.data = inf->buffer;

//	std::cout << "sherpa_msg: Reading from port" << std::endl;
	int read_bytes = __port_read(port, &msg);
	if (read_bytes <= 0) {
//		std::cout << "sherpa_msg: No data recieved from port" << std::endl;
		return;
	}

	std::cout << "sherpa_msg: read bytes = " << read_bytes << std::endl;

  /* Either decode or encode message */


  /* Write result back t0 port */
	ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
	ubx_data_t result_msg;
//	result_msg.data = (void *)zframe_data(frame);
//	result_msg.len = zframe_size(frame);
	result_msg.type = type;

	//hexdump((unsigned char *)result_msg.data, result_msg.len, 16);
	__port_write(inf->ports.msg_out, &result_msg);

}




