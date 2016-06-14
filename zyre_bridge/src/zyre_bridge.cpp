#include "zyre_bridge.hpp"
#include <zyre.h>
#include <stdlib.h>
#include <iostream>


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

static void zyre_bridge_actor(zsock_t *pipe, void *args);

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct zyre_bridge_info
{
        /* add custom block local data here */
		// List of accepted msg types
		char *type_list;

		// max number of msg to send per step
		int *max_send;

		// zyre node
		zyre_t *node;
		// zyre groups
		char *group;

        // actor running in separate thread to receive the messages
		zactor_t* actor;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct zyre_bridge_port_cache ports;
};

/* init */
int zyre_bridge_init(ubx_block_t *b)
{
        int ret = -1;
        struct zyre_bridge_info *inf;
        unsigned int tmplen;
	
        /* allocate memory for the block local state */
        if ((inf = (struct zyre_bridge_info*)calloc(1, sizeof(struct zyre_bridge_info)))==NULL) {
                ERR("zyre_bridge: failed to alloc memory");
                ret=EOUTOFMEM;
                goto out;
        }

        update_port_cache(b, &inf->ports);

        int *max_send;
        max_send = (int*) ubx_config_get_data_ptr(b, "max_send", &tmplen);
        printf("max_send value for block %s is %d\n", b->name, *max_send);
        inf->max_send = max_send;

        ///TODO: need ot get a list of type names in here
        char *type_list;
        type_list = (char*) ubx_config_get_data_ptr(b, "type_list", &tmplen);
        printf("List of types for block %s is %s\n", b->name, type_list);
        inf->type_list = type_list;

        int major, minor, patch;
        zyre_version (&major, &minor, &patch);
        if (major != ZYRE_VERSION_MAJOR)
        	goto out;
        if (minor != ZYRE_VERSION_MINOR)
        	goto out;
        if (patch != ZYRE_VERSION_PATCH)
        	goto out;

        ///TODO: do sanity checks on all the config items
        char *wm_name;
        wm_name = (char*) ubx_config_get_data_ptr(b, "wm_name", &tmplen);
        printf("zyre name for block %s is %s\n", b->name, wm_name);
        zyre_t *node;
        node = zyre_new (wm_name);
        if (!node){
        	printf("Could not create a zyre node!");
        	goto out;
        }
        inf->node = node;

        int rc;
        char *loc_ep;
        char *gos_ep;
        loc_ep = (char*) ubx_config_get_data_ptr(b, "local_endpoint", &tmplen);
        gos_ep = (char*) ubx_config_get_data_ptr(b, "gossip_endpoint", &tmplen);
        printf("local and gossip endpoint for block %s is %s and %s\n", b->name, loc_ep, gos_ep);
		rc = zyre_set_endpoint (node, "%s",loc_ep);
		assert (rc == 0);
		//  Set up gossip network for this node
		///TODO: add a check that rhere is an endpoint
		zyre_gossip_connect (node, "%s",gos_ep);
		rc = zyre_start (node);
		assert (rc == 0);

		///TODO: enable list of group names
		char *group;
		group = (char*) ubx_config_get_data_ptr(b, "group", &tmplen);
		zyre_join (node, group);
		inf->group = group;

		//  Give time for them to interconnect
		zclock_sleep (100);

        b->private_data=inf;
        ret=0;
out:
        return ret;
}

/* start */
int zyre_bridge_start(ubx_block_t *b)
{
        struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
	
        // incoming data is handled by the actor thread
        zactor_t* actor = zactor_new (zyre_bridge_actor, b);
        assert(actor);
        inf->actor = actor;

        int ret = 0;
        return ret;
}

/* stop */
void zyre_bridge_stop(ubx_block_t *b)
{
         struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
		// clean up actor thread
		zactor_destroy(&inf->actor);
		printf("Destroyed zactor!\n");
}

/* cleanup */
void zyre_bridge_cleanup(ubx_block_t *b)
{
		struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;

		//zyre_t *node = inf->node;
		zyre_stop (inf->node);
		zclock_sleep (100);
		zyre_destroy (&inf->node);

		free(b->private_data);
		printf("Cleanup successful.\n");
}

/* step */
void zyre_bridge_step(ubx_block_t *b)
{

    struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;

	/* Read data from port */
	ubx_port_t* port = inf->ports.zyre_out;
	assert(port != 0);

	ubx_data_t msg;
	checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
	msg.type = port->in_type;
	///TODO: get length from config
	unsigned char *test = new unsigned char [2000];
	msg.len = 2000;
	msg.data = test;

	//	std::cout << "zmq_sender: Reading from port" << std::endl;
	int read_bytes = __port_read(port, &msg);
	if (read_bytes <= 0) {
		//printf("zyre_bridge: No data recieved from port\n");
		return;
	}

	printf("zyre_bridge: read bytes = %d\n",read_bytes);
	//char bla[2000];
	//sprintf(bla,"%d",msg.data);
	//printf("msg: %s\n", bla);
	char *bla = (char*) msg.data;
	//std::cout << "Received " << bla << std::endl;
	zyre_shouts(inf->node, inf->group, "%s", bla);
	printf("Update shouted! \n\n");


}


static void
zyre_bridge_actor (zsock_t *pipe, void *args)
{
    // initialization
    ubx_block_t *b = (ubx_block_t *) args;
    struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
    printf("zyre_bridge: actor started.\n");
    // send signal on pipe socket to acknowledge initialization
    zsock_signal (pipe, 0);

    bool terminated = false;
	zpoller_t *poller = zpoller_new (pipe, zyre_socket (inf->node), NULL);
	while (!terminated) {
		void *which = zpoller_wait (poller, -1);
		if (which == pipe) {
			zmsg_t *msg = zmsg_recv (which);
			if (!msg)
				break;              //  Interrupted

			char *command = zmsg_popstr (msg);
			if (streq (command, "$TERM"))
				terminated = true;
//			else
//			if (streq (command, "SHOUT")) {
//				char *string = zmsg_popstr (msg);
//				printf("Internal msg through pipe: %s",string);
//				//zyre_shouts (inf->node, "CHAT", "%s", string);
//			}
			else {
				puts ("Invalid pipe message to actor");
			}
			free (command);
			zmsg_destroy (&msg);
		}
		else
		if (which == zyre_socket (inf->node)) {
			zmsg_t *msg = zmsg_recv (which);
			char *event = zmsg_popstr (msg);
			char *peer = zmsg_popstr (msg);
			char *name = zmsg_popstr (msg);
			char *group = zmsg_popstr (msg);
			char *message = zmsg_popstr (msg);

			if (streq (event, "ENTER"))
				printf ("%s has joined the chat\n", name);
			else
			if (streq (event, "EXIT"))
				printf ("%s has left the chat\n", name);
			else
			if (streq (event, "SHOUT")) {
				printf ("%s: SHOUT received.\n", name);

				ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
				ubx_data_t ubx_msg;
				ubx_msg.data = (void *)message;
				//printf("message: %s\n",message);
				ubx_msg.len = strlen(message);
				ubx_msg.type = type;
				__port_write(inf->ports.zyre_in, &ubx_msg);
			}
			else
			if (streq (event, "WHISPER"))
				printf ("%s: WHISPER %s\n", name, message);
			else
			if (streq (event, "EVASIVE"))
				printf ("%s is being evasive\n", name);

			free (event);
			free (peer);
			free (name);
			free (group);
			free (message);
			zmsg_destroy (&msg);
		}
	}
	zpoller_destroy (&poller);
	zclock_sleep (100);
}
