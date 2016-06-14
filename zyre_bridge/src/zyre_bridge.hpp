/*
 * zyre_bridge microblx function block (autogenerated, don't edit)
 */

#include <ubx.h>

/* includes types and type metadata */

ubx_type_t types[] = {
        { NULL },
};

/* block meta information */
char zyre_bridge_meta[] =
        " { doc='Zyre bridge',"
        "   real-time=false,"
        "}";

ubx_config_t zyre_bridge_config[] = {
        { .name="max_send", .type_name = "int", .doc="Max number of msgs that are sent during step fcn." },
		{ .name="type_list", .type_name = "char", .doc="list of relevant msg types that are forwarded to RSG" },
		{ .name="wm_name", .type_name = "char", .doc="name of the bridge in the zyre network." },
		{ .name="local_endpoint", .type_name = "char", .doc="local endpoint for zyre network" },
		{ .name="gossip_endpoint", .type_name = "char", .doc="endpoint for zyre gossip discovery" },
		{ .name="group", .type_name = "char", .doc="zyre group to join" },
		{ .name="bind", .type_name = "int", .doc="decides whether this node binds or connects to gossip network" },
		{ NULL },
};

/* declaration port block ports */
ubx_port_t zyre_bridge_ports[] = {
        { .name="zyre_in", .out_type_name="unsigned char", .doc="Received msg from zyre."  },
		{ .name="zyre_out", .in_type_name="unsigned char", .doc="Sends zyre msg."  },
		{ NULL },
};

/* declare a struct port_cache */
struct zyre_bridge_port_cache {
        ubx_port_t* zyre_in;
        ubx_port_t* zyre_out;
};

/* declare a helper function to update the port cache this is necessary
 * because the port ptrs can change if ports are dynamically added or
 * removed. This function should hence be called after all
 * initialization is done, i.e. typically in 'start'
 */
static void update_port_cache(ubx_block_t *b, struct zyre_bridge_port_cache *pc)
{
        pc->zyre_in = ubx_port_get(b, "zyre_in");
        pc->zyre_out = ubx_port_get(b, "zyre_out");
}


/* for each port type, declare convenience functions to read/write from ports */
//def_read_fun(read_zmq_in, unsigned char)

/* block operation forward declarations */
int zyre_bridge_init(ubx_block_t *b);
int zyre_bridge_start(ubx_block_t *b);
void zyre_bridge_stop(ubx_block_t *b);
void zyre_bridge_cleanup(ubx_block_t *b);
void zyre_bridge_step(ubx_block_t *b);


/* put everything together */
ubx_block_t zyre_bridge_block = {
        .name = "zyre_bridge",
        .type = BLOCK_TYPE_COMPUTATION,
        .meta_data = zyre_bridge_meta,
        .configs = zyre_bridge_config,
        .ports = zyre_bridge_ports,

        /* ops */
        .init = zyre_bridge_init,
        .start = zyre_bridge_start,
        .stop = zyre_bridge_stop,
        .cleanup = zyre_bridge_cleanup,
        .step = zyre_bridge_step,
};


/* zyre_bridge module init and cleanup functions */
int zyre_bridge_mod_init(ubx_node_info_t* ni)
{
        DBG(" ");
        int ret = -1;
        ubx_type_t *tptr;

        for(tptr=types; tptr->name!=NULL; tptr++) {
                if(ubx_type_register(ni, tptr) != 0) {
                        goto out;
                }
        }

        if(ubx_block_register(ni, &zyre_bridge_block) != 0)
                goto out;

        ret=0;
out:
        return ret;
}

void zyre_bridge_mod_cleanup(ubx_node_info_t *ni)
{
        DBG(" ");
        const ubx_type_t *tptr;

        for(tptr=types; tptr->name!=NULL; tptr++)
                ubx_type_unregister(ni, tptr->name);

        ubx_block_unregister(ni, "zyre_bridge");
}

/* declare module init and cleanup functions, so that the ubx core can
 * find these when the module is loaded/unloaded */
UBX_MODULE_INIT(zyre_bridge_mod_init)
UBX_MODULE_CLEANUP(zyre_bridge_mod_cleanup)
