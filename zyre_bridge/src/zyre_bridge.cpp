#include "zyre_bridge.hpp"
#include <zyre.h>


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
        printf("configuration for block %s is %d\n", b->name, *max_send);
        inf->max_send = max_send;

        char *type_list;
        type_list = (char*) ubx_config_get_data_ptr(b, "type_list", &tmplen);
        printf("configuration for block %s is %s\n", b->name, type_list);
        inf->type_list = type_list;

        int major, minor, patch;
        zyre_version (&major, &minor, &patch);
        if (major != ZYRE_VERSION_MAJOR)
        	goto out;
        if (minor != ZYRE_VERSION_MINOR)
        	goto out;
        if (patch != ZYRE_VERSION_PATCH)
        	goto out;

        char *wm_name;
        wm_name = (char*) ubx_config_get_data_ptr(b, "wm_name", &tmplen);
        printf("zyre name for block %s is %s\n", b->name, wm_name);
        zyre_t *zyre;
        zyre = zyre_new (wm_name);

        /*
         * TODO
         * Replace below code with ZYRE init code.
         */

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
	inf->actor = actor;

        int ret = 0;
        return ret;
}

/* stop */
void zyre_bridge_stop(ubx_block_t *b)
{
        /* struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data; */
}

/* cleanup */
void zyre_bridge_cleanup(ubx_block_t *b)
{
		struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
		// clean up actor thread
		zactor_destroy(&inf->actor);
		free(b->private_data);
}

/* step */
void zyre_bridge_step(ubx_block_t *b)
{

        //struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;

}

int handle_event(zloop_t *loop, zsock_t *reader, void *args) {
         // initialization
        ubx_block_t *b = (ubx_block_t *) args;
        //struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
        printf("zyre_bridge: data available.\n");

    	/*
    	 * TODO:
    	 * Replace blow code with ZYRE code for receiving a WHISPER or SHOUT message.
    	 *
    	 */


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
        //__port_write(inf->ports.zmq_in, &msg);

        /* Inform potential observers ? */

        // clean up temporary frame object
        zframe_destroy (&frame);

        return 1;
}


static void
zyre_bridge_actor (zsock_t *pipe, void *args)
{
    // initialization
    //ubx_block_t *b = (ubx_block_t *) args;
    //struct zyre_bridge_info *inf = (struct zyre_bridge_info*) b->private_data;
    printf("zyre_bridge: actor started.\n");
    // send signal on pipe socket to acknowledge initialisation
    zsock_signal (pipe, 0);

//    zloop_t *loop = zloop_new ();
//    assert (loop);
//    int rc = zloop_reader (loop, inf->subscriber, handle_event, args);
//    assert (rc == 0);
//    zloop_start (loop);
//
//    zloop_destroy (&loop);
}
