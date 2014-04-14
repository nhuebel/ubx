#include "cpp_receiver.hpp"

#include <iostream>
using namespace std;

UBX_MODULE_LICENSE_SPDX(GPL-2.0+)

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct cpp_receiver_info
{
        /* add custom block local data here */

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct cpp_receiver_port_cache ports;
};

/* init */
int cpp_receiver_init(ubx_block_t *b)
{
        int ret = -1;
        struct cpp_receiver_info *inf;

        /* allocate memory for the block local state */
        if ((inf = (struct cpp_receiver_info*)calloc(1, sizeof(struct cpp_receiver_info)))==NULL) {
                ERR("cpp_receiver: failed to alloc memory");
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
int cpp_receiver_start(ubx_block_t *b)
{
        /* struct cpp_receiver_info *inf = (struct cpp_receiver_info*) b->private_data; */
        int ret = 0;
        return ret;
}

/* stop */
void cpp_receiver_stop(ubx_block_t *b)
{
        /* struct cpp_receiver_info *inf = (struct cpp_receiver_info*) b->private_data; */
}

/* cleanup */
void cpp_receiver_cleanup(ubx_block_t *b)
{
        free(b->private_data);
}

/* step */
void cpp_receiver_step(ubx_block_t *b)
{
        struct cpp_receiver_info *inf = (struct cpp_receiver_info*) b->private_data;
	struct cpp_data dat;
	read_input(inf->ports.input, &dat);
	cout << "MODEL: " << dat.model << endl;
	cout << "UID: " << dat.uid << endl;
	cout << "META MODEL: " << dat.meta_model << endl;
	cout << "DATA: " << dat.data << endl;
}

