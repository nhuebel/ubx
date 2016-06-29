#include "zyre_bridge.hpp"
#include <zyre.h>
#include <jansson.h>
#include <vector>
#include <string>

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
		int max_send;
		// max number of msg to send per step
		unsigned long max_msg_length;
//        // Msg buffer for input port
//		unsigned char* msg_buffer;

		//list of msg types that are accepted by the zyre bridge and handed to the RSG and length of list
		std::vector<std::string> input_type_list ;
		//list of msg types that coming from the RSG and will be broadcasted to local components
		std::vector<std::string> output_type_list ;

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
                return ret;
        }

        update_port_cache(b, &inf->ports);

        //If the following code is used, do not forget to also use the code freeing the memory in the cleanup. And also to put it into the bridge_info struct
//        inf->msg_buffer = (unsigned char*) malloc(inf->max_msg_length*sizeof(unsigned char*));
//        if (!inf->msg_buffer){
//        	printf("%s: Could not allocate memory for msg buffer. \n", b->name);
//        	goto out;
//        }

		int *max_msg_length;
        max_msg_length = (int*) ubx_config_get_data_ptr(b, "max_msg_length", &tmplen);
		printf("max_msg_length value for block %s is %d\n", b->name, *max_msg_length);
		inf->max_msg_length = *max_msg_length;
		if (inf->max_msg_length <=0){
			printf("ERR: %s: max_msg_length must be >0!\n",b->name);
			return ret;
		}

        int *max_send;
        max_send = (int*) ubx_config_get_data_ptr(b, "max_send", &tmplen);
        printf("max_send value for block %s is %d\n", b->name, *max_send);
        inf->max_send = *max_send;

//        ///TODO: need to get a list of type names in here
//        char *type_list;
//        type_list = (char*) ubx_config_get_data_ptr(b, "type_list", &tmplen);
//        printf("List of types for block %s is %s\n", b->name, type_list);
//        inf->type_list = type_list;
        ///TODO: for now hardcoded list -> fix, read from comma separated string
        inf->input_type_list.push_back("RSGUpdate_local");
        inf->input_type_list.push_back("RSGUpdate_both");
        inf->input_type_list.push_back("RSGQuery");
        inf->input_type_list.push_back("RSGFunctionBlock");

        int major, minor, patch;
        zyre_version (&major, &minor, &patch);
        if (major != ZYRE_VERSION_MAJOR)
        	return ret;
        if (minor != ZYRE_VERSION_MINOR)
        	return ret;
        if (patch != ZYRE_VERSION_PATCH)
        	return ret;

        char *wm_name;
        wm_name = (char*) ubx_config_get_data_ptr(b, "wm_name", &tmplen);
        printf("zyre name for block %s is %s\n", b->name, wm_name);
        zyre_t *node;
        node = zyre_new (wm_name);
        if (!node){
        	printf("Could not create a zyre node!");
        	return ret;
        }
        inf->node = node;

        int rc;
        //char *loc_ep;
        char *gos_ep;
        //loc_ep = (char*) ubx_config_get_data_ptr(b, "local_endpoint", &tmplen);
        gos_ep = (char*) ubx_config_get_data_ptr(b, "gossip_endpoint", &tmplen);
        //printf("local and gossip endpoint for block %s is %s and %s\n", b->name, loc_ep, gos_ep);
		//rc = zyre_set_endpoint (node, "%s",loc_ep);
		//assert (rc == 0);
		//  Set up gossip network for this node
        ubx_data_t *dmy;
		dmy = ubx_config_get_data(b, "bind");
		int bind;
		bind = *(int*) dmy->data;
		if (bind == 1) {
			printf("%s: This block will bind to gossip endpoint '%s'\n", b->name,gos_ep);
			zyre_gossip_bind (node, "%s", gos_ep);
		} else if (bind == 0) {
			printf("%s: This block will connect to gossip endpoint '%s' \n", b->name,gos_ep);
			zyre_gossip_connect (node, "%s",gos_ep);
		} else {
			printf("%s: Wrong value for bind configuration. Must be 0 or 1. \n", b->name);
			return ret;
		}
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

		//free(inf->msg_buffer);
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

	char * tmp_str = (char*) malloc(inf->max_msg_length*sizeof(char*));

	ubx_data_t msg;
	checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
	msg.type = port->in_type;
	msg.len = inf->max_msg_length;
	msg.data = tmp_str;
	//msg.data = inf->msg_buffer;

    int counter = 0;
    while (counter < inf->max_send) {
    	int read_bytes = __port_read(port, &msg);

    	//printf("zyrebidge: read bytes: %d\n",read_bytes);
    	//printf("step: read strlen: %lu\n",strlen((char*) msg.data));

    	if (read_bytes <= 0) {
    		//printf("zyre_bridge: No data recieved from port\n");
    		goto out;
    	}
    	// port_read returns byte array. Need to add 0 termination manually to the string.
    	tmp_str[read_bytes] = '\0';

    	// create json object and embed it into msg envelope
    	json_t *pl;
		json_error_t error;
		pl= json_loads(tmp_str,0,&error);
		if(!pl) {
			printf("Error parsing JSON payload! line %d: %s\n", error.line, error.text);
			json_decref(pl);
			return;
		}

		printf("zyrebidge: sending msg: %s\n", tmp_str);
    	zyre_shouts(inf->node, inf->group, "%s", tmp_str);
    	counter++;
   }

out:
    free(tmp_str);
    return;
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
		// handle msgs from main thread
		if (which == pipe) {
			zmsg_t *msg = zmsg_recv (which);
			if (!msg)
				break;              //  Interrupted
			// only react to TERM signal
			char *command = zmsg_popstr (msg);
			if (streq (command, "$TERM"))
				terminated = true;
			else {
				puts ("Invalid pipe message to actor");
			}
			free (command);
			zmsg_destroy (&msg);
		} else
		// handle msgs from zyre network
		if (which == zyre_socket (inf->node)) {
			zmsg_t *msg = zmsg_recv (which);
			if (!msg) {
				printf("zyre_bridge: interrupted!\n");
			}
			char *event = zmsg_popstr (msg);
			char *peer = zmsg_popstr (msg);
			char *name = zmsg_popstr (msg);

			if (streq (event, "ENTER"))
				printf ("zyre_bridge: %s has entered\n", name);
			else
			if (streq (event, "EXIT"))
				printf ("zyre_bridge: %s has exited\n", name);
			else
			if (streq (event, "SHOUT")) {
				printf ("zyre_bridge: SHOUT received from %s.\n", name);

				char *group = zmsg_popstr (msg);
				char *message = zmsg_popstr (msg);

				// load JSON msg
		    	json_t *m;
				json_error_t error;
				m= json_loads(message,0,&error);
				if(!m) {
					printf("Error parsing JSON payload! line %d: %s\n", error.line, error.text);
					json_decref(m);
					return;
				}
			    if (json_object_get(m, "type")) {
			    	for (int i=0; i < inf->input_type_list.size();i++)
			    	{
						if (json_string_value(json_object_get(m, "type")) == inf->input_type_list[i]){
							ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
							ubx_data_t ubx_msg;
							ubx_msg.data = (void *)message;
							//printf("message: %s\n",message);
							ubx_msg.len = strlen(message);
							ubx_msg.type = type;
							__port_write(inf->ports.zyre_in, &ubx_msg);
						}
			    	}
			    } else {
			    	printf("Error parsing JSON string! Does not conform to msg model.\n");
			    	return;
			    }


				free (group);
				free (message);
			}
			else
			if (streq (event, "WHISPER")){
				char *message = zmsg_popstr (msg);
				printf ("%s: WHISPER \n%s\n", name, message);
				free (message);
			}
			else
			if (streq (event, "EVASIVE"))
				printf ("zyre_bridge: %s is being evasive\n", name);

			free (event);
			free (peer);
			free (name);
			zmsg_destroy (&msg);
		}
	}
	zpoller_destroy (&poller);
	//TODO: make parametrizable
	zclock_sleep (100);
}
