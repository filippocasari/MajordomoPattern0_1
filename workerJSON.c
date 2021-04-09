

#include <mdp.h>

#include <json-c/json.h>

#define NUM_WORKERS 1
#define BROKER_ENDPOINT  "tcp://127.0.0.1:5000"

#define VENDOR "Renault"
#define POWER "90cv"


static void workerTask(zsock_t *pipe, void *args);

zmsg_t *handle_type_request(zframe_t *request[]);

double speed = 130.0;

int main() {

    zactor_t *workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers[i] = zactor_new(workerTask, NULL);
        if(zctx_interrupted){
            break;
        }
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        zactor_destroy(&workers[i]);
    }


    return 0;
}

static void
workerTask(zsock_t *pipe, void *args) {

    int verbose = 1; //set if you wanna get logger


    //create a new worker, you have to pass "endpoint of the broker", "service required" and the verbose
    mdp_worker_t *session = mdp_worker_new(
            BROKER_ENDPOINT, "engine_1", verbose);


    mdp_worker_set_heartbeat(session,
                             7500); //set the heartbeat time. After this time in seconds, worker will send to worker an heartbeat message


    zframe_t *reply_to; // this frame identifies from who (which client) the message is
    zmsg_t *reply_message; // create a new message for reply



    //until exception
    int count = 0;
    srand(time(NULL));
    while (1) {
        zsys_catch_interrupts();
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            mdp_worker_destroy(&session);
            zclock_sleep(500);
            break;
        }

        zmsg_t *request = mdp_worker_recv(session,
                                          &reply_to); // receive the request message body, it can be 1) Null if there isn't the body, one or more frames

        count++;                                                       //internally this function pop all previous frames before the request body frames
        printf("REQUESTS RECEIVED: %d\n", count);
        //@ MESSAGE WORKER MAJORDOMO 0.1
        //************************************************************************
        //*                          EMPTY FRAME    (DELIMITER)                  *
        //*                          6 BYTE FOR WORKER VERSION                   *
        //*                          1 BYTE FOR COMMAND                          *
        //*                          CLIENT ADDRESS                              *
        //*                          EMPTY FRAME     (DELIMITER)                 *
        //*                          REQUEST FRAMES                              *
        //************************************************************************


        int size_request = zmsg_size(request); //SIZE of the request

        //frames of body request
        zframe_t *request_stream[size_request];

        printf("RECEIVED MESSAGE FROM BROKER\n");

        //handle no body request
        if (size_request == 0) {
            printf("No Body request\n");
            zframe_destroy(request_stream);
            break;
        }
        puts("REQUEST BODY FRAMES");
        printf("NUMBER OF FRAMES: %d\n", size_request);

        char *s = NULL; //string extracted from each frame

        // extract frames "for loop"
        for (int i = 0; i < size_request; i++) {
            //create a list of frames and for each one extract the string
            request_stream[i] = zmsg_pop(request);
            //duplicating string of the frame into the printable string
            s = zframe_strdup(request_stream[i]);
            printf("BODY FRAME[%d]: %s\n", i, s);

            printf("FRAME TYPE-CONTENT: %s\n", "PLAIN STRING");
            printf("(BYTE) SIZE FRAME: %lu\n", sizeof(s) * strlen(s));
        }
        //this is reply message initialization
        reply_message = zmsg_new();
        //return the reply
        reply_message = handle_type_request(request_stream);


        //send to broker the reply if exists
        mdp_worker_send(session, &reply_message, reply_to);

    }
    mdp_worker_destroy(&session); //free instance

}

zmsg_t *handle_type_request(zframe_t *request[]) {

    size_t n = (int) (sizeof(request) / sizeof(request[0]));
    zframe_t *frame_n;
    zmsg_t *reply = zmsg_new();
    json_object *REQ[n];
    json_object *REP[n];
    for (int i = 0; i < n; i++) {
        frame_n = request[i];
        char *request_string_json = zframe_strdup(frame_n);
        zframe_destroy(&frame_n);

        REQ[i] = json_tokener_parse(request_string_json);


    }

    for (int i = 0; i < n; i++) {
        printf("REQUEST BODY FRAME [%d] : \n", i);
        json_object_object_foreach(REQ[i], key, val) {
            printf("\t%s: %s\n", key, json_object_to_json_string(val));
            REP[i] = json_object_new_object();


            puts("client wants the speed!");
            json_object_object_add(REP[i], "VENDOR", json_object_new_string(VENDOR));
            json_object_object_add(REP[i], "POWER", json_object_new_string(POWER));

            speed += (double) rand() / RAND_MAX * 2.0 - 1.0;
            json_object_object_add(REP[i], "VALUE", json_object_new_double(speed));
            int64_t timestamp = zclock_time();
            json_object_object_add(REP[i], "timestamp", json_object_new_int64(timestamp));


        }
        printf("REPLY [%d] =\n", i);
        json_object_object_foreach(REP[i], key2, val2) {
            printf("\t%s: %s\n", key2, json_object_to_json_string(val2));
        }
        puts("\n\n");

        const char *reply_string = json_object_to_json_string(REP[i]);
        zmsg_pushstr(reply, reply_string);


    }

    return reply;
}