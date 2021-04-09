#include <msgpack.h>
#include <stdio.h>
#include <mdp.h>
#define BROKER_ENDPOINT "tcp://192.168.0.113:5000"

#define REQUEST "GET"

int main(void) {

    /* creates buffer and serializer instance. */
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    /* serializes ["Hello", "MessagePack"]. */
    msgpack_pack_array(pk, 3);
    msgpack_pack_bin(pk, 9);
    msgpack_pack_bin_body(pk, "TYPE: REQ", 9);
    msgpack_pack_bin(pk, 13);
    msgpack_pack_bin_body(pk, "REQ_TYPE: GET", 13);
    msgpack_pack_bin(pk, 13);
    msgpack_pack_bin_body(pk, "SENSOR: SPEED", 13);

    zmsg_t *reply = NULL; //reply client will receive
    zmsg_t *request = zmsg_new(); // request initialization
    int64_t start; //start time to see processing time
    int64_t end; //end time

    mdp_client_t *session2 = mdp_client_new(BROKER_ENDPOINT, 1);

    //start the time

    start = zclock_mono();
    // send all request without wait the reply==> ZMQ doc calls this Asynchronous Client

    for (int count = 0; count < 50; count++) {

        int succ = zmsg_pushmem(request, pk->data, buffer->size);
        //push the string set before into the request message
        // handle error
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            break;
        }
        if (succ == -1) {
            puts("ERROR ");
        }
        //send request to broker for service "coffee" in this case
        mdp_client_send(session2, "engine_1", &request);
        // reinitialize request
        request = zmsg_new();
    }


    /* deserializes it. */
    puts("-----------------------------------------------------------------------");
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return ret = msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL);

    /* prints the deserialized object. */
    msgpack_object obj = msg.data;
    msgpack_object_print(stdout, obj);/*=> ["Hello", "MessagePack"] */

    /* cleaning */
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);
    mdp_client_destroy(&session2);










}

