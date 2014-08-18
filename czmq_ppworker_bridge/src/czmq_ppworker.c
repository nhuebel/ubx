#include "czmq_ppworker.h"

//  Paranoid Pirate worker

#include "czmq.h"
#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  1000    //  msecs
#define INTERVAL_INIT       1000    //  Initial reconnect
#define INTERVAL_MAX       32000    //  After exponential backoff

//  Paranoid Pirate Protocol constants
#define PPP_READY       "\001"      //  Signals worker is ready
#define PPP_HEARTBEAT   "\002"      //  Signals worker heartbeat

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct czmq_ppworker_info
{
        /* add custom block local data here */

        // actor running in separate thread to receive the messages
		zactor_t* actor;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct czmq_ppworker_port_cache ports;
};

//  Helper function that returns a new configured socket
//  connected to the Paranoid Pirate queue

static zsock_t *
s_worker_socket () {
	zsock_t *worker = zsock_new_dealer("tcp://localhost:5556");

    //  Tell queue we're ready for work
    printf ("I: worker ready\n");
    zframe_t *frame = zframe_new (PPP_READY, 1);
    zframe_send (&frame, worker, 0);

    return worker;
}

//  .split main task
//  We have a single task that implements the worker side of the
//  Paranoid Pirate Protocol (PPP). The interesting parts here are
//  the heartbeating, which lets the worker detect if the queue has
//  died, and vice versa:
static void ppworker_actor(zsock_t *pipe, void *args)
{
	ubx_block_t *b = (ubx_block_t *) args;
	struct czmq_ppworker_info *inf = (struct czmq_ppworker_info*) b->private_data;
    zsock_t *worker = s_worker_socket ();

    //  If liveness hits zero, queue is considered disconnected
    size_t liveness = HEARTBEAT_LIVENESS;
    size_t interval = INTERVAL_INIT;

    //  Send out heartbeats at regular intervals
    uint64_t heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;

    srandom ((unsigned) time (NULL));

    printf("ppworker: actor started.\n");
    // send signal on pipe socket to acknowledge initialisation
    zsock_signal (pipe, 0);

    while (true) {
        zmq_pollitem_t items [] = { { zsock_resolve(worker),  0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

        if (items [0].revents & ZMQ_POLLIN) {
            //  Get message
            //  - 3-part envelope + content -> request
            //  - 1-part HEARTBEAT -> heartbeat
            zmsg_t *msg = zmsg_recv (worker);
            if (!msg)
                break;          //  Interrupted

            if (zmsg_size (msg) == 3) {
                printf ("I: normal reply\n");
                byte *buffer;
                size_t buffer_size = zmsg_encode (msg, &buffer);
                ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
                ubx_data_t umsg;
                umsg.data = (void *)buffer;
                umsg.len = buffer_size;
                umsg.type = type;
                __port_write(inf->ports.zmq_in, &umsg);
                zmsg_send (&msg, worker);
                liveness = HEARTBEAT_LIVENESS;
                sleep (1);              //  Do some heavy work
                if (zsys_interrupted)
                    break;
            }
            else
            //  .split handle heartbeats
            //  When we get a heartbeat message from the queue, it means the
            //  queue was (recently) alive, so we must reset our liveness
            //  indicator:
            if (zmsg_size (msg) == 1) {
                zframe_t *frame = zmsg_first (msg);
                if (memcmp (zframe_data (frame), PPP_HEARTBEAT, 1) == 0)
                    liveness = HEARTBEAT_LIVENESS;
                else {
                    printf ("E: invalid message\n");
                    zmsg_dump (msg);
                }
                zmsg_destroy (&msg);
            }
            else {
                printf ("E: invalid message\n");
                zmsg_dump (msg);
            }
            interval = INTERVAL_INIT;
        }
        else
        //  .split detecting a dead queue
        //  If the queue hasn't sent us heartbeats in a while, destroy the
        //  socket and reconnect. This is the simplest most brutal way of
        //  discarding any messages we might have sent in the meantime:
        if (--liveness == 0) {
            printf ("W: heartbeat failure, can't reach queue\n");
            printf ("W: reconnecting in %zd msec...\n", interval);
            zclock_sleep (interval);

            if (interval < INTERVAL_MAX)
                interval *= 2;
            zsock_destroy(&worker);
            worker = s_worker_socket ();
            liveness = HEARTBEAT_LIVENESS;
        }
        //  Send heartbeat to queue if it's time
        if (zclock_time () > heartbeat_at) {
            heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;
            printf ("I: worker heartbeat\n");
            zframe_t *frame = zframe_new (PPP_HEARTBEAT, 1);
            zframe_send (&frame, worker, 0);
        }
    }
}




/* init */
int czmq_ppworker_init(ubx_block_t *b)
{
        int ret = -1;
        struct czmq_ppworker_info *inf;

        /* allocate memory for the block local state */
        if ((inf = (struct czmq_ppworker_info*)calloc(1, sizeof(struct czmq_ppworker_info)))==NULL) {
                ERR("czmq_ppworker: failed to alloc memory");
                ret=EOUTOFMEM;
                goto out;
        }
        b->private_data=inf;
        update_port_cache(b, &inf->ports);
        ret=0;
out:
        return ret;
}

/* start */
int czmq_ppworker_start(ubx_block_t *b)
{
        struct czmq_ppworker_info *inf = (struct czmq_ppworker_info*) b->private_data;
        int ret = 0;
	// incoming data is handled by the actor thread
        zactor_t* actor = zactor_new (ppworker_actor, b);
        assert(actor);
        inf->actor = actor;
        return ret;
}

/* stop */
void czmq_ppworker_stop(ubx_block_t *b)
{
        /* struct czmq_ppworker_info *inf = (struct czmq_ppworker_info*) b->private_data; */
}

/* cleanup */
void czmq_ppworker_cleanup(ubx_block_t *b)
{
        free(b->private_data);
}

/* step */
void czmq_ppworker_step(ubx_block_t *b)
{
        /*
        struct czmq_ppworker_info *inf = (struct czmq_ppworker_info*) b->private_data;
        */
}

