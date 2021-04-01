
#include <mdp.h>
#include <uuid/uuid.h>

// raspberry endpoint : "tcp://192.168.0.113:5000"
//localhost : "tcp://127.0.0.1:5000"
#define BROKER_ENDPOINT  "tcp://192.168.0.113:5000"
#define NUM_CLIENTS 1

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
    //zsock_signal(pipe, 0);
    int verbose = 1;
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse_upper(uuid, uuid_str);
    zclock_log("UUID CLIENT: %s", uuid_str);
    mdp_client_t *session = mdp_client_new(BROKER_ENDPOINT, verbose);


    //mettere commenti
    int count;
    zmsg_t *reply = NULL;
    zmsg_t *request = zmsg_new();
    int64_t start;
    int64_t end;
    start = zclock_time();
    char *command;
    char *service;
    for (count = 0; count < 200; count++) {
        request = zmsg_new();
        assert(request);
        int succ = zmsg_pushstr(request, "I wanna a coffee"); // aggiungere JSON/CBOR
        if (succ == -1) {
            zclock_log("ERROR ");
        }
        mdp_client_send(session, "coffee", &request);

        reply = mdp_client_recv(session, &command, &service);

        if (reply == NULL || command!=) {
            zsys_error("No reply...destroying the client...");
            mdp_client_destroy(&session);

        }
    }
    end = zclock_time() - start;

    zclock_log("%d requests/replies processed", count);
    zclock_log("Time for Synchronous Client is : %ld ms\n", end);
    mdp_client_destroy(&session);

}
