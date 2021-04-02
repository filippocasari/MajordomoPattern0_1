
#include <mdp.h>


// raspberry endpoint : "tcp://192.168.0.113:5000"
//localhost : "tcp://127.0.0.1:5000"
#define BROKER_ENDPOINT  "tcp://192.168.0.113:5000"
#define NUM_CLIENTS 1
#define TYPE_REQUEST 0
static void
client_task(zsock_t *pipe, void *args);

int main(int argc, char *argv[]) {
    //int verbose = (argc > 1 && streq (argv[1], "-v"));
    zactor_t *clients[NUM_CLIENTS];
    int i = 0;
    for (; i < NUM_CLIENTS; i++) {
        clients[i] = zactor_new(client_task, NULL);
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
        }

    }
    for (i = 0; i < NUM_CLIENTS; i++) {
        zactor_destroy(&clients[i]);
    }
    zactor_destroy(clients);


    return 0;
}


static void
client_task(zsock_t *pipe, void *args) {
    mdp_client_t *session2 = mdp_client_new(BROKER_ENDPOINT, 0);



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
        char *command; //command received
        char *service; // from which service
        zmsg_t *reply2 = mdp_client_recv(session2,&command, &service); //reply if any

        //if reply is null, just tell to stdout
        if (reply2 == NULL) {
            puts("NO REPLY...");
        }
        zmsg_destroy(&reply2);

        // reinitialize request
        request = zmsg_new();
    }

    //dealloc any msg or string
    free(request_str);
    zmsg_destroy(&request);
    zmsg_destroy(&reply);
    end = zclock_mono() - start;

    //print how many requests client tried to send and how much time has just spent on it

    printf("%d requests/replies processed\n", count);
    printf("Time for synchronous Client is : %ld ms", end);
    mdp_client_destroy(&session2); //destroy and free memory

}
