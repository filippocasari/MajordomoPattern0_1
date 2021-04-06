#include <stdio.h>
#include "include/broker_mdp_extended.h"
#include <mdp.h>
int main(int argc, char *argv[]) {
    int verbose = 0;
    int daemonize = 0;
    int user_endpoint = 0;
    for (int i = 1; i < argc; i++)
    {
        if (streq(argv[i], "-v")) verbose = 1;
        else if (streq(argv[i], "-d")) daemonize = 1;
        else if (streq(argv[i], "-h"))
        {
            printf("%s [-h] | [-d] [-v] [broker url]\n\t-h This help message\n\t-d Daemon mode.\n\t-v Verbose output\n\tbroker url defaults to tcp://*:5555\n", argv[0]);
            return -1;
        }
        else user_endpoint = 1;
    }

    if (daemonize != 0)
    {
        int rc = daemon(0, 0);
        assert (rc == 0);
    }

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
        s_broker_bind (self, "tcp://127.0.0.1:5000");
        printf("Bound to tcp://127.0.0.1:5000\n");
    }

    int rc=s_broker_loop(self);



    if(rc>0){
        zsys_error("\nerror in the broker or interrupt command...\nRETURN CODE:%d ", rc);
    }
    return 0;
}
