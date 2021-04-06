
#include <mdp.h>
#include "include/client_task_asynchronous.h"

#include "include/coffeeType.h"
// raspberry endpoint : "tcp://192.168.0.113:5000"
//localhost : "tcp://127.0.0.1:5000"


#define NUM_CLIENTS 1



int main(int argc, char *argv[]) {
    int verbose = (argc > 1 && streq (argv[1], "-v"));
    verbose = 1; //verbose if you wanna get a logger
    int i = 0;
    zactor_t *clients[NUM_CLIENTS];
    for (; i < NUM_CLIENTS; i++) {
        clients[i] = zactor_new(client_asynchronous_task, NULL);
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


