//
// Created by Filippo on 06/04/2021.
//

#ifndef MAJORDOMOPATTERN0_1_CLIENT_TASK_ASYNCHRONOUS_H
#define MAJORDOMOPATTERN0_1_CLIENT_TASK_ASYNCHRONOUS_H

#define BROKER_ENDPOINT  "tcp://127.0.0.1:5000"
#define VERBOSE 1
#define TYPE_REQUEST 0 //kind of coffee you want to require

static void
client_asynchronous_task(zsock_t *pipe, void *args) {

    //create a new client and automatically connect with broker endpoint
    mdp_client_t *session2 = mdp_client_new(BROKER_ENDPOINT, VERBOSE);



    int count; // number of request
    zmsg_t *reply = NULL; //reply client will receive
    zmsg_t *request = zmsg_new(); // request initialization
    int64_t start; //start time to see processing time
    int64_t end; //end time

    // setting client request string
    int length = snprintf( NULL, 0, "%d", TYPE_REQUEST);
    char* request_str = malloc( length + 1 );
    snprintf( request_str, length + 1, "%d", TYPE_REQUEST );

    //start the time

    start= zclock_mono();
    // send all request without wait the reply==> ZMQ doc calls this Asynchronous Client

    for (count = 0; count < 50; count++) {

        int succ = zmsg_pushstr(request, request_str); //push the string set before into the request message
        // handle error
        if (succ == -1) {
            puts("ERROR ");
        }
        //send request to broker for service "coffee" in this case
        mdp_client_send(session2, "coffee", &request);
        // reinitialize request
        request = zmsg_new();
    }

    //dealloc any msg or string
    free(request_str);
    zmsg_destroy(&request);
    zmsg_destroy(&reply);

    //for loop to receive reply messages
    for (count = 0; count < 50; count++) {
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            break;
        }
        char *command; //command received
        char *service; // from which service
        zmsg_t *reply2 = mdp_client_recv(session2,&command, &service); //reply if any

        //if reply is null, just tell to stdout
        if (reply2 == NULL) {
            puts("NO REPLY...");
        }
        zmsg_destroy(&reply2);
    }
    // end time
    end = zclock_mono() - start;

    //print how many requests client tried to send and how much time has just spent on it

    printf("%d requests/replies processed\n", count);
    printf("Time for Asynchronous Client is : %ld ms", end);
    mdp_client_destroy(&session2); //destroy and free memory
}



#endif //MAJORDOMOPATTERN0_1_CLIENT_TASK_ASYNCHRONOUS_H
