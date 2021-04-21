
#include <stdio.h>
#include <mdp.h>
#include "mdp_common.h"
#include <czmq.h>
#include "coffeeType.h"

// this is the task which runs a single worker
//
// this worker catches messages for the coffee service
//




#ifndef MAJORDOMEPATTERNSAMPLE_WORKERTASK_H
#define MAJORDOMEPATTERNSAMPLE_WORKERTASK_H
// raspberry endpoint : "tcp//:192.168.0.113:5001"
//localhost : "tcp//:127.0.0.1:5000"
#define BROKER_ENDPOINT_SECURE  "tcp://192.168.0.113:5001"
#define BROKER_ENDPOIN_PLAIN "tcp://127.0.0.1:5000"



//handle the kind of request or the stream request (if there are more then 1 frame)
// and tries to match according to type of coffee
zmsg_t* handle_type_request(zframe_t *request[]);

//create a random string
char *rand_string(char *str, size_t size) {
    unsigned char data[size];
    FILE *fp;
    fp = fopen("/dev/urandom", "r");
    fread(&data, 1, size, fp);
    fclose(fp);
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            unsigned char key = data[n] % (unsigned char) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

// #TODO pass to args if worker task has to connect to secure or plain port
static void
workerTask(zsock_t *pipe, void *args) {
    char identity[32];
    char random_part[5];
    strcpy(identity, "coffeeService_");
    rand_string(random_part, 5);
    strcat(identity, random_part);
    zclock_log("Worker %s", identity);  //create an unique identity for each instance worker but it's just to recognize the worker thread;
                                                // it could be useful to send identity to broker (optional)
    int verbose = 1; //set if you wanna get logger


    //create a new worker, you have to pass "endpoint of the broker", "service required" and the verbose
    mdp_worker_t *session = mdp_worker_new(
            BROKER_ENDPOIN_PLAIN, "coffee", verbose);


    mdp_worker_set_heartbeat(session, 7500); //set the heartbeat time. After this time in seconds, worker will send to worker an heartbeat message


    zframe_t *reply_to; // this frame identifies from who (which client) the message is
    zmsg_t *reply_message; // create a new message for reply



    //until exception
    int count=0;
    while (1) {

        zsys_catch_interrupts(); // if any interrupt caught, show it

        zmsg_t *request = mdp_worker_recv(session, &reply_to); // receive the request message body, it can be 1) Null if there isn't the body, one or more frames

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


        int size_request = (int) zmsg_size(request); //SIZE of the request

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
            printf("(BYTE) SIZE FRAME: %lu\n", sizeof(s)*strlen(s));
        }
        //this is reply message initialization
        reply_message = zmsg_new();
        //return the reply
        reply_message=handle_type_request(request_stream);


        //send to broker the reply if exists
        mdp_worker_send(session, &reply_message, reply_to);

    }
    mdp_worker_destroy(&session); //free instance

}

zmsg_t* handle_type_request(zframe_t *request[]){
    zframe_t *first_frame=request[0]; //the first frame identifies the request in this case, but it can be customized

    zmsg_t *reply=zmsg_new(); //create a new local reply, it can be replaced by passing a pointer from the caller to not use other memory

    char *type=zframe_strdup(first_frame); //string of the type of coffee

    int int_type=atoi(type); //convert to int to use switch case , atoi deprecated, better to get a function to handle errors

    // in any case, push this string into a reply

    //cases of type coffee received
    switch (int_type) {
        case AMERICAN_COFFEE:
            zmsg_pushstr(reply, "AMERICAN COFFEE");
            break;
        case EXPRESS_COFFEE:
            zmsg_pushstr(reply,"EXPRESS COFFEE");
            break;
        case MOCHA:
            zmsg_pushstr(reply, "MOCHA");
            break;
        case MACCHIATO:
            zmsg_pushstr(reply, "MACCHIATO");
            break;
        default:
            zsys_error("NO MATCHING ANY TYPE OF COFFEE");
            return NULL;
    }
    zmsg_pushstr(reply, "here your ");
    return reply;

}


#endif //MAJORDOMEPATTERNSAMPLE_WORKERTASK_H
