#include <zyre.h>
#include <jansson.h>
#include <uuid/uuid.h>
#include <string.h>

typedef struct _json_msg_t {
    char *metamodel;
    char *model;
    char *type;
    char *payload;
} json_msg_t;

typedef struct _query_t {
        const char *uid;
        const char *requester;
        json_msg_t *msg;
        zactor_t *loop;
} query_t;

typedef struct _component_t {
	const char* name;
	const char *localgroup;
	zyre_t *local;
	json_t *config;
	zpoller_t *poller;
	zlist_t *query_list;
	int timeout;
	int no_of_updates;
	int no_of_queries;
	int no_of_fcn_block_calls;
	int alive;
} component_t;

void query_destroy (query_t **self_p) {
        assert (self_p);
        if(*self_p) {
            query_t *self = *self_p;
            free (self);
            *self_p = NULL;
        }
}

void destroy_component (component_t **self_p) {
    assert (self_p);
    if(*self_p) {
    	component_t *self = *self_p;
        zyre_destroy (&self->local);
        json_decref(self->config);
        zpoller_destroy (&self->poller);
        //free memory of all items from the query list
        query_t *it;
        while(zlist_size (self->query_list) > 0){
        	it = zlist_pop (self->query_list);
        	query_destroy(&it);
        }
        zlist_destroy (&self->query_list);
        free (self);
        *self_p = NULL;
    }
}

query_t * query_new (const char *uid, const char *requester, json_msg_t *msg, zactor_t *loop) {
        query_t *self = (query_t *) zmalloc (sizeof (query_t));
        if (!self)
            return NULL;
        self->uid = uid;
        self->requester = requester;
        self->msg = msg;
        self->loop = loop;

        return self;
}

component_t* new_component(json_t *config) {
	component_t *self = (component_t *) zmalloc (sizeof (component_t));
    if (!self)
        return NULL;
    if (!config)
            return NULL;

    self->config = config;

	self->name = json_string_value(json_object_get(config, "short-name"));
    if (!self->name) {
        destroy_component (&self);
        return NULL;
    }
	self->timeout = json_integer_value(json_object_get(config, "timeout"));
    if (self->timeout <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_updates = json_integer_value(json_object_get(config, "no_of_updates"));
    if (self->no_of_updates <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_queries = json_integer_value(json_object_get(config, "no_of_queries"));
    if (self->no_of_queries <= 0) {
    	destroy_component (&self);
        return NULL;
    }

	self->no_of_fcn_block_calls = json_integer_value(json_object_get(config, "no_of_fcn_block_calls"));
    if (self->no_of_fcn_block_calls <= 0) {
    	destroy_component (&self);
        return NULL;
    }
	//  Create local gossip node
	self->local = zyre_new (self->name);
    if (!self->local) {
    	destroy_component (&self);
        return NULL;
    }
	printf("[%s] my local UUID: %s\n", self->name, zyre_uuid(self->local));

	/* config is a JSON object */
	// set values for config file as zyre header.
	const char *key;
	json_t *value;
	json_object_foreach(config, key, value) {
		const char *header_value;
		if(json_is_string(value)) {
			header_value = json_string_value(value);
		} else {
			header_value = json_dumps(value, JSON_ENCODE_ANY);
		}
		printf("header key value pair\n");
		printf("%s %s\n",key,header_value);
		zyre_set_header(self->local, key, "%s", header_value);
	}

	int rc;
	if(!json_is_null(json_object_get(config, "gossip_endpoint"))) {
		//  Set up gossip network for this node
		zyre_gossip_connect (self->local, "%s", json_string_value(json_object_get(config, "gossip_endpoint")));
		printf("[%s] using gossip with gossip hub '%s' \n", self->name,json_string_value(json_object_get(config, "gossip_endpoint")));
	} else {
		printf("[%s] WARNING: no local gossip communication is set! \n", self->name);
	}
	rc = zyre_start (self->local);
	assert (rc == 0);
	self->localgroup = strdup("local");
	zyre_join (self->local, self->localgroup);
	//  Give time for them to connect
	zclock_sleep (1000);

	//create a list to store queries...
	self->query_list = zlist_new();
	if ((!self->query_list)&&(zlist_size (self->query_list) == 0)) {
		destroy_component (&self);
		return NULL;
	}

	self->alive = 1; //will be used to quit program after answer to query is received
	self->poller =  zpoller_new (zyre_socket(self->local), NULL);
	return self;
}

json_t * load_config_file(char* file) {
    json_error_t error;
    json_t * root;
    root = json_load_file(file, JSON_ENSURE_ASCII, &error);
    printf("[%s] config file: %s\n", json_string_value(json_object_get(root, "short-name")), json_dumps(root, JSON_ENCODE_ANY));
    if(!root) {
   	printf("Error parsing JSON file! line %d: %s\n", error.line, error.text);
    	return NULL;
    }
    return root;
}


int decode_json(char* message, json_msg_t *result) {
	/**
	 * decodes a received msg to json_msg types
	 *
	 * @param received msg as char*
	 * @param json_msg_t* at which the result is stored
	 *
	 * @return returns 0 if successful and -1 if an error occurred
	 */
    json_t *root;
    json_error_t error;
    root = json_loads(message, 0, &error);

    if(!root) {
    	printf("Error parsing JSON string! line %d: %s\n", error.line, error.text);
    	return -1;
    }

    if (json_object_get(root, "metamodel")) {
    	result->metamodel = strdup(json_string_value(json_object_get(root, "metamodel")));
    } else {
    	printf("Error parsing JSON string! Does not conform to msg model.\n");
    	return -1;
    }
    if (json_object_get(root, "model")) {
		result->model = strdup(json_string_value(json_object_get(root, "model")));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    if (json_object_get(root, "type")) {
		result->type = strdup(json_string_value(json_object_get(root, "type")));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    if (json_object_get(root, "payload")) {
    	result->payload = strdup(json_dumps(json_object_get(root, "payload"), JSON_ENCODE_ANY));
	} else {
		printf("Error parsing JSON string! Does not conform to msg model.\n");
		return -1;
	}
    json_decref(root);
    return 0;
}

//char* send_msg() {
//	/**
//	 * creates a world model update msg
//	 *
//	 * @return the string encoded JSON msg that can be sent directly via zyre. Must be freed by user! Returns NULL if wrong json types are passed in.
//	 */
//
//    json_t *pl;
//    pl = json_object();
//    json_object_set(pl, "@worldmodeltype", json_string("RSGUpdate"));
//    json_object_set(pl, "operation", json_string("CREATE"));
//	json_object_set(pl, "parentId", json_string("e379121f-06c6-4e21-ae9d-ae78ec1986a1"));
//    json_t *node;
//    node = json_object();
//    json_object_set(node, "@graphtype", json_string("Node"));
//	zuuid_t *uuid = zuuid_new ();
//	assert(uuid);
//	//printf("UUID: %s\n",zuuid_str_canonical(uuid));
//    json_object_set(node, "id", json_string(zuuid_str_canonical(uuid)));
//	json_t *attributes;
//    attributes = json_array();
//	json_t *kv;
//	kv = json_object();
//	json_object_set(kv, "key", json_string("name"));
//	json_object_set(kv, "value", json_string("myNode"));
//	json_array_append_new(attributes, kv);
//	kv = json_object();
//	json_object_set(kv, "key", json_string("comment"));
//	json_object_set(kv, "value", json_string("Some human readable comment on the node."));
//	json_array_append_new(attributes, kv);
//	json_object_set(node, "attributes", attributes);
//    json_object_set(pl, "node", node);
//    char* ret = json_dumps(pl, JSON_ENCODE_ANY);
//
//	json_decref(kv);
//	json_decref(attributes);
//	json_decref(node);
//    json_decref(pl);
//    return ret;
//}

char* send_query(component_t* self, char* query_type, json_t* query_params) {
	/**
	 * creates a query msg for the world model and adds it to the query list
	 *
	 * @param string query_type as string containing one of the availble query types ["GET_NODES", "GET_NODE_ATTRIBUTES", "GET_NODE_PARENTS", "GET_GROUP_CHILDREN", "GET_ROOT_NODE", "GET_REMOTE_ROOT_NODES", "GET_TRANSFORM", "GET_GEOMETRY", "GET_CONNECTION_SOURCE_IDS", "GET_CONNECTION_TARGET_IDS"]
	 * @param json_t query_params json object containing all information required by the query; check the rsg-query-schema.json for details
	 *
	 * @return the string encoded JSON msg that can be sent directly via zyre. Must be freed by user! Returns NULL if wrong json types are passed in.
	 */

	// create the payload, i.e., the query
    json_t *pl;
    pl = json_object();
    json_object_set(pl, "@worldmodeltype", json_string("RSGQuery"));
    json_object_set(pl, "query", json_string(query_type));
	zuuid_t *uuid = zuuid_new ();
	assert(uuid);
    json_object_set(pl, "queryId", json_string(zuuid_str_canonical(uuid)));
	
	if (json_object_size(query_params)>0) {
		const char *key;
		json_t *value;
		json_object_foreach(query_params, key, value) {
			json_object_set(pl, key, value);
		}
	}
	// pack it into the standard msg envelope
	json_t *env;
    env = json_object();
	json_object_set(env, "metamodel", json_string("SHERPA"));
	json_object_set(env, "model", json_string("RSGQuery"));
	json_object_set(env, "type", json_string("RSGQuery"));
	json_object_set(env, "payload", pl);
	
	// add it to the query list
	json_msg_t *msg = (json_msg_t *) zmalloc (sizeof (json_msg_t));
	msg->metamodel = strdup("SHERPA");
	msg->model = strdup("RSGQuery");
	msg->type = strdup("RSGQuery");
	msg->payload = strdup(json_dumps(pl, JSON_ENCODE_ANY));
	query_t * q = query_new(zuuid_str_canonical(uuid), zyre_uuid(self->local), msg, NULL);
	zlist_append(self->query_list, q);

    char* ret = json_dumps(env, JSON_ENCODE_ANY);
	
	json_decref(env);
    json_decref(pl);
    return ret;
}


int main(int argc, char *argv[]) {

    int major, minor, patch;
    zyre_version (&major, &minor, &patch);
    assert (major == ZYRE_VERSION_MAJOR);
    assert (minor == ZYRE_VERSION_MINOR);
    assert (patch == ZYRE_VERSION_PATCH);

    // load configuration file
    json_t * config = load_config_file("test_config.json");
    if (config == NULL) {
      return -1;
    }
    component_t *self = new_component(config);
    if (self == NULL) {
    	return -1;
    }
    printf("[%s] component initialised!\n", self->name);

    //check if there is at least one component connected
    zlist_t * tmp = zlist_new ();
    assert (tmp);
    assert (zlist_size (tmp) == 0);
    // timestamp for timeout
    struct timespec ts = {0,0};
    if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
		printf("[%s] Could not assign time stamp!\n",self->name);
	}
    struct timespec curr_time = {0,0};
    while (true) {
    	printf("[%s] Checking for connected peers.\n",self->name);
    	tmp = zyre_peers(self->local);
    	printf("[%s] %zu peers connected.\n", self->name,zlist_size(tmp));
    	if (zlist_size (tmp) > 0)
    		break;
		if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
			// if timeout, stop component
			double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
			double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
			if (curr_time_msec - ts_msec > self->timeout) {
				printf("[%s] Timeout! Could not connect to other peers.\n",self->name);
				zyre_stop (self->local);
				printf ("[%s] Stopping...", self->name);
				zclock_sleep (100);
				zyre_destroy (&self->local);
				printf ("[%s] SHUTDOWN\n", self->name);
				return 0;
			}
		} else {
			printf ("[%s] could not get current time\n", self->name);
		}
    	zclock_sleep (1000);
    }
    zlist_destroy(&tmp);

    printf("\n");
	printf("#########################################\n");
	printf("[%s] Sending Query for RSG Root Node\n",self->name);
	printf("#########################################\n");
	printf("\n");
	json_t* query_params = NULL;
	char *msg = send_query(self,"GET_ROOT_NODE",query_params);
	zyre_shouts(self->local, self->localgroup, "%s", msg);
	printf("[%s] Sent msg: %s \n",self->name,msg);
	printf("[%s] Queries in queue: %d \n",self->name,zlist_size (self->query_list));
	//wait for query to be answered
	//while (zlist_size (self->query_list) > 0){
		void *which = zpoller_wait (self->poller, ZMQ_POLL_MSEC);
		//if (which) {
	//}




//	void *which = zpoller_wait (poller, ZMQ_POLL_MSEC);
//	if (which) {
//		printf("[%s] local data received!\n", self);
//		zmsg_t *msg = zmsg_recv (which);
//		if (!msg) {
//			printf("[%s] interrupted!\n", self);
//			return -1;
//		}
//		//reset timeout
//		if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
//			printf("[%s] Could not assign time stamp!\n",self);
//		}
//		char *event = zmsg_popstr (msg);
//
//		if (streq (event, "WHISPER")) {
//			 assert (zmsg_size(msg) == 3);
//			 char *peerid = zmsg_popstr (msg);
//			 char *name = zmsg_popstr (msg);
//			 char *message = zmsg_popstr (msg);
//			 printf ("[%s] %s %s %s %s\n", self, event, peerid, name, message);
//			 //printf("[%s] Received: %s from %s\n",self, event, name);
//			json_msg_t *result = (json_msg_t *) zmalloc (sizeof (json_msg_t));
//			if (decode_json(message, result) == 0) {
//				// load the payload as json
//				json_t *payload;
//				json_error_t error;
//				payload= json_loads(result->payload,0,&error);
//				if(!payload) {
//					printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
//				} else {
//					const char *uid = json_string_value(json_object_get(payload,"UID"));
//					//TODO:does this string need to be freed?
//					if (!uid){
//						printf("[%s] Received msg without UID!\n", self);
//					} else {
//						// search through stored list of queries and check if this query corresponds to one we have sent
//						query_t *it = zlist_first(query_list);
//						int found_UUID = 0;
//						while (it != NULL) {
//							if streq(it->uid, uid) {
//								printf("[%s] Received reply to query %s.\n", self, uid);
//								if (streq(result->type,"peer-list")){
//									printf("Received peer list: %s\n",result->payload);
//									//TODO: search list for a wasp
//									alive = 0;
//
//								} else if (streq(result->type,"communication_report")){
//									printf("Received communication_report: %s\n",result->payload);
//									/////////////////////////////////////////////////
//									//Do something with the report
//									if (json_is_true(json_object_get(payload,"success"))){
//										printf("Yeay! All recipients have received the msg.\n");
//									} else {
//										printf("Sending msg was not successful because of: %s\n",json_string_value(json_object_get(payload,"error")));
//									}
//									/////////////////////////////////////////////////
//
//									if (streq(json_string_value(json_object_get(payload,"error")),"Unknown recipients")){
//										//This is really not how coordination in a program should be done -> TODO: clean up
//
//									}
//
//								}
//								found_UUID = 1;
//								zlist_remove(query_list,it);
//								//TODO: make sure the data of that query is properly freed
//							}
//							it = zlist_next(query_list);
//						}
//						if (found_UUID == 0) {
//							printf("[%s] Received a msg with an unknown UID!\n", self);
//						}
//					}
//					json_decref(payload);
//				}
//
//
//
//			} else {
//				printf ("[%s] message could not be decoded\n", self);
//			}
//			free(result);
//			zstr_free(&peerid);
//			zstr_free(&name);
//			zstr_free(&message);
//		} else {
//			printf ("[%s] received %s msg\n", self, event);
//		}
//		zstr_free (&event);
//		zmsg_destroy (&msg);
//	} else {
//		if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
//			// if timeout, stop component
//			double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
//			double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
//			if (curr_time_msec - ts_msec > timeout) {
//				printf("[%s] Timeout! No msg received for %i msec.\n",self,timeout);
//				break;
//			}
//		} else {
//			printf ("[%s] could not get current time\n", self);
//		}
//		printf ("[%s] waiting for a reply. Could execute other code now.\n", self);
//		zclock_sleep (1000);
//	}



/**TODO:
 * * test why there is no query result with sebastian
 * * wait for return of query result to know root node
 * * send some updates -> write update function
 * * send some some queries -> send_query is written
 * * send some fnction_block_calls -> write function for that
 * * store all these queries
 * * wait till all queries are resolved
 * * make sure that some return errors
 */


//	int i;
//    for( i = 0; i < no_of_updates; i++){
//		printf("\n");
//		printf("#########################################\n");
//		printf("[%s] Sending Update %d\n",self,i);
//		printf("#########################################\n");
//		printf("\n");
//		char *msg = send_msg();
//		if (msg) {
//			zyre_shouts(local, localgroup, "%s", msg);
//			printf("[%s] Sent msg: %s \n",self,msg);
//			zstr_free(&msg);
//		} else {
//			alive = false;
//		}
//		free(msg);
//	}
	
//	for( i = 0; i < no_of_queries; i++){
//		printf("\n");
//		printf("#########################################\n");
//		printf("[%s] Sending Query %d\n",self,i);
//		printf("#########################################\n");
//		printf("\n");
////		char *msg = send_query();
//		char *msg = send_msg();
//		if (msg) {
//			zyre_shouts(local, localgroup, "%s", msg);
//			printf("[%s] Sent msg: %s \n",self,msg);
//			zstr_free(&msg);
//		} else {
//			alive = false;
//		}
//		free(msg);
//	}
	
//	for( i = 0; i < no_of_fcn_block_calls; i++){
//		printf("\n");
//		printf("#########################################\n");
//		printf("[%s] Sending Function Block Call %d\n",self,i);
//		printf("#########################################\n");
//		printf("\n");
////		char *msg = send_query();
//		char *msg = send_msg();
//		if (msg) {
//			zyre_shouts(local, localgroup, "%s", msg);
//			printf("[%s] Sent msg: %s \n",self,msg);
//			zstr_free(&msg);
//		} else {
//			alive = false;
//		}
//		free(msg);
//	}


//     	void *which = zpoller_wait (poller, ZMQ_POLL_MSEC);
//     	if (which) {
// 			printf("[%s] local data received!\n", self);
// 			zmsg_t *msg = zmsg_recv (which);
// 			if (!msg) {
// 				printf("[%s] interrupted!\n", self);
// 				return -1;
// 			}
// 			//reset timeout
// 			if (clock_gettime(CLOCK_MONOTONIC,&ts)) {
// 				printf("[%s] Could not assign time stamp!\n",self);
// 			}
// 			char *event = zmsg_popstr (msg);
// 			
// 			if (streq (event, "WHISPER")) {
//                 assert (zmsg_size(msg) == 3);
//                 char *peerid = zmsg_popstr (msg);
//                 char *name = zmsg_popstr (msg);
//                 char *message = zmsg_popstr (msg);
//                 printf ("[%s] %s %s %s %s\n", self, event, peerid, name, message);
//                 //printf("[%s] Received: %s from %s\n",self, event, name);
// 				json_msg_t *result = (json_msg_t *) zmalloc (sizeof (json_msg_t));
// 				if (decode_json(message, result) == 0) {
// 					// load the payload as json
// 					json_t *payload;
// 					json_error_t error;
// 					payload= json_loads(result->payload,0,&error);
// 					if(!payload) {
// 						printf("Error parsing JSON send_remote! line %d: %s\n", error.line, error.text);
// 					} else {
// 						const char *uid = json_string_value(json_object_get(payload,"UID"));
// 						//TODO:does this string need to be freed?
// 						if (!uid){
// 							printf("[%s] Received msg without UID!\n", self);
// 						} else {
// 							// search through stored list of queries and check if this query corresponds to one we have sent
// 							query_t *it = zlist_first(query_list);
// 							int found_UUID = 0;
// 							while (it != NULL) {
// 								if streq(it->UID, uid) {
// 									printf("[%s] Received reply to query %s.\n", self, uid);
// 									if (streq(result->type,"peer-list")){
// 										printf("Received peer list: %s\n",result->payload);
// 										//TODO: search list for a wasp
// 										alive = 0;
// 
// 									} else if (streq(result->type,"communication_report")){
// 										printf("Received communication_report: %s\n",result->payload);
// 										/////////////////////////////////////////////////
// 										//Do something with the report
// 										if (json_is_true(json_object_get(payload,"success"))){
// 											printf("Yeay! All recipients have received the msg.\n");
// 										} else {
// 											printf("Sending msg was not successful because of: %s\n",json_string_value(json_object_get(payload,"error")));
// 										}
// 										/////////////////////////////////////////////////
// 
// 										if (streq(json_string_value(json_object_get(payload,"error")),"Unknown recipients")){
// 											//This is really not how coordination in a program should be done -> TODO: clean up
// 						
// 										}
// 
// 									}
// 									found_UUID = 1;
// 									zlist_remove(query_list,it);
// 									//TODO: make sure the data of that query is properly freed
// 								}
// 								it = zlist_next(query_list);
// 							}
// 							if (found_UUID == 0) {
// 								printf("[%s] Received a msg with an unknown UID!\n", self);
// 							}
// 						}
// 						json_decref(payload);
// 					}
// 
// 
// 
// 				} else {
// 					printf ("[%s] message could not be decoded\n", self);
// 				}
// 				free(result);
// 				zstr_free(&peerid);
// 				zstr_free(&name);
// 				zstr_free(&message);
// 			} else {
// 				printf ("[%s] received %s msg\n", self, event);
// 			}
// 			zstr_free (&event);
// 			zmsg_destroy (&msg);
//     	} else {
// 			if (!clock_gettime(CLOCK_MONOTONIC,&curr_time)) {
// 				// if timeout, stop component
// 				double curr_time_msec = curr_time.tv_sec*1.0e3 +curr_time.tv_nsec*1.0e-6;
// 				double ts_msec = ts.tv_sec*1.0e3 +ts.tv_nsec*1.0e-6;
// 				if (curr_time_msec - ts_msec > timeout) {
// 					printf("[%s] Timeout! No msg received for %i msec.\n",self,timeout);
// 					break;
// 				}
// 			} else {
// 				printf ("[%s] could not get current time\n", self);
// 			}
//     		printf ("[%s] waiting for a reply. Could execute other code now.\n", self);
//     		zclock_sleep (1000);
//     	}



    zyre_stop (self->local);
	printf ("[%s] Stopping zyre.\n", self->name);
	zclock_sleep (100);
    zyre_destroy (&self->local);
    printf ("[%s] Destroying component.\n", self->name);
    destroy_component(&self);
    //  @end
    printf ("SHUTDOWN\n");
    return 0;
}


