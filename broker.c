#include <stdio.h>
#include "include/broker_mdp_extended.h"
#include <mdp.h>
#define BROKER_ENDPOINT  "tcp://192.168.1.7:5000"
// "tcp://192.168.1.7:5000" "tcp://127.0.0.1:5000"

int main(int argc, char *argv[]) {
    int verbose = 0;
    int daemonize = 0;
    int user_endpoint = 0;

    long start_time_creation;
    long end_time_creation;


    for (int i = 1; i < argc; i++)
    {
        if (streq(argv[i], "-v")) verbose = 1;
        else if (streq(argv[i], "-d")) daemonize = 1;
        else if (streq(argv[i], "-h"))
        {
            printf("%s [-h] | [-d] [-v] [broker url]\n\t-h This help message\n\t-d Daemon mode.\n\t-v Verbose output\n\tbroker url defaults to tcp://*:5000\n", argv[0]);
            return -1;
        }
        else user_endpoint = 1;
    }

    if (daemonize != 0)
    {
        int rc = daemon(0, 0);
        assert (rc == 0);
    }
    start_time_creation=zclock_usecs();
    broker_t *self = s_broker_new (verbose);

    /* did the user specify a bind address? */
    if (user_endpoint == 1)
    {
        s_broker_bind (self, argv[argc-1]);
        printf("Bound to %s\n", argv[argc-1]);
    }
    else
    {
        /* default */
        s_broker_bind (self, BROKER_ENDPOINT);
        printf("Bound to %s\n", BROKER_ENDPOINT);
    }
    end_time_creation=zclock_usecs();
    long time_of_creation_broker= end_time_creation-start_time_creation;
    if(time_of_creation_broker>0){
        printf("TIME FOR CREATION AND BINDING OF BROKER : %ld [micro secs]\n", time_of_creation_broker);
    }
    else{
        puts("NO TIME OF CREATION BECAUSE TIME IS NEGATIVE\n");
    }
    int rc=s_broker_loop(self);



    if(rc>0){

        zsys_error("\nerror in the broker or interrupt command...\nRETURN CODE:%d ", rc);
        long start_time_to_close_broker=zclock_usecs();
        s_broker_destroy(&self);
        long end_time_to_close_broker=zclock_usecs();
        printf("END TIME TO CLOSE BROKER: %ld [micro secs]\n", end_time_to_close_broker-start_time_to_close_broker);
    }
    return 0;
}
