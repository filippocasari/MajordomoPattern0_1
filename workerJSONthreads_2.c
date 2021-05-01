//
// Created by utente on 27/04/2021.
//


#include <mdp.h>

#include <json-c/json.h>

#include "Queue/queue.h"


#define NUM_WORKERS 1
// "tcp://192.168.0.113:5000"
// "tcp://127.0.0.1:5000"
//"tcp://192.168.1.7:5000"
#define BROKER_ENDPOINT  "tcp://127.0.0.1:5000"

#define VENDOR "Renault"
#define POWER "90cv"


static void workerTask(zsock_t *pipe, void *args);

void *consumer(void *arg);

void *producer(void *arg);

zmsg_t *handle_type_request(zmsg_t *request);

void print_parsing_time(const long *start, const long *end);

void print_average_parsing_time(const long *parsing_array, const size_t *num_samples);

int check_option(const int *argc, char *argv[], int *daemonize, int *verbose, int *user_endpoint);


struct counter_t {
    int value;
    pthread_mutex_t lock;
};

static struct counter_t num_replies_consum;
static struct counter_t num_replies_prod;


double speed = 130.0;

#define BUFFER_SIZE 100

struct Queue *queue;


mdp_worker_t *session;

// Number of producer threads to start
// Must be greater than zero
#define NUM_OF_PRODUCERS 5
#define NUM_OF_CONSUMERS 5


int main(int argc, char *argv[]) {
    int verbose = 1;
    int daemonize = 0;
    int user_endpoint = 0;
    check_option(&argc, argv, &daemonize, &verbose, &user_endpoint);

    zactor_t *workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (streq(argv[0], "-v")) verbose = 1;
        workers[i] = zactor_new(workerTask, &verbose);

    }
    zsys_catch_interrupts();
    if (zsys_interrupted) {
        for (int i = 0; i < NUM_WORKERS; i++) {
            zactor_destroy(&workers[i]);
        }
    }
    for (int i = 0; i < NUM_WORKERS; i++) {
        zactor_destroy(&workers[i]);
    }


    return 0;
}

int check_option(const int *argc, char *argv[], int *daemonize, int *verbose, int *user_endpoint) {
    for (int i = 1; i < *argc; i++) {
        if (streq(argv[i], "-v")) *verbose = 1;
        else if (streq(argv[i], "-d")) *daemonize = 1;
        else if (streq(argv[i], "-h")) {
            printf("%s [-h] | [-d] [-v] [broker url]\n\t-h This help message\n\t-d Daemon mode.\n\t-v Verbose output\n\tbroker url defaults to tcp://*:5000\n",
                   argv[0]);
            return -1;
        } else *user_endpoint = 1;
    }

    if (*daemonize != 0) {
        int rc = daemon(0, 0);
        assert (rc == 0);
    }
    return 0;
}

static void
workerTask(zsock_t *pipe, void *args) {

    queue = newQueue(100);


    //TODO managing of arguments!!
    zsys_catch_interrupts();
    int *arguments = (int *) args;
    int verbose = arguments[0];

    long end;

    long time_to_signup;
    char *endpoint = BROKER_ENDPOINT;
    long start = zclock_usecs();
    session = mdp_worker_new(
            endpoint, "engine_1", verbose);
    end = zclock_usecs();
    time_to_signup = end - start;
    printf("TIME TO SIGN UP: %ld [micro secs]\n", time_to_signup);
    mdp_worker_set_heartbeat(session,
                             7500); //set the heartbeat time. After this time in seconds, worker will send to worker an heartbeat message

    srand(time(NULL));
    pthread_t consumers[NUM_OF_CONSUMERS];

    int num = 0;
    while (!zctx_interrupted) {


        zframe_t *reply_to;
        zmsg_t *request = mdp_worker_recv(session, &reply_to);
        //replies_to[NUM_OF_PRODUCERS % 50] = reply_to;
        //incoda
        enqueue(queue, request);
        //pthread_create(&producers[num % NUM_OF_PRODUCERS], NULL, &producer, &request);
        //printf("Starting Thread producer %d\n", num % NUM_OF_PRODUCERS);
        pthread_create(&consumers[num % NUM_OF_CONSUMERS], NULL, consumer, &reply_to);
        printf("Starting Thread consumer %d\n", num % NUM_OF_CONSUMERS);

        pthread_join(consumers[num % NUM_OF_CONSUMERS], NULL);
        zmsg_destroy(&request);
        num++;


    }
    mdp_worker_destroy(&session);


}

zmsg_t *handle_type_request(zmsg_t *request) {

    zmsg_t *reply = zmsg_new();

    json_object *REP;
    puts("building a reply...");

    REP = json_object_new_object();

    json_object_object_add(REP, "VENDOR", json_object_new_string(VENDOR));
    json_object_object_add(REP, "POWER", json_object_new_string(POWER));

    speed += (double) rand() / RAND_MAX * 2.0 - 1.0;
    json_object_object_add(REP, "VALUE", json_object_new_double(speed));

    int64_t timestamp = zclock_time();

    json_object_object_add(REP, "timestamp", json_object_new_int64(timestamp));

    puts("REPLY =" );
    json_object_object_foreach(REP, key2, val2) {
        printf("\t%s: %s\n", key2, json_object_to_json_string(val2));
    }
    puts("\n\n");

    const char *reply_string = json_object_to_json_string(REP);
    zmsg_pushstr(reply, reply_string);




    return reply;
}

void print_average_parsing_time(const long *parsing_array, const size_t *num_samples) {
    long sum = 0;
    for (int i = 0; i < *num_samples; i++) {
        sum += parsing_array[i];
    }
    long double average = (long double) sum / *num_samples;
    printf("AVERAGE TIME OF PARSING FOR WORKER: %Lf [micro secs]\n", average);
}

void print_parsing_time(const long *start, const long *end) {
    if ((end - start) > 0) {
        printf("PARSING TIME: %ld [micro secs]\n", end - start);
    } else {
        puts("CANNOT PARSING, TIME IS NEGATIVE...\n");
    }

}

void *producer(void *arg) {

    zmsg_t *request = (zmsg_t *) arg;
    zmsg_print(request);
    request = arg;


    enqueue(queue, request);
    zmsg_destroy(&request);


    return NULL;
}

void *consumer(void *arg) {


    zframe_t *reply_to;
    reply_to = (zframe_t *) arg;
    zmsg_t *request;
    request =  dequeue(queue);


    //increment(&num_replies_consum);
    int size_request = (int) zmsg_size(request);

    zframe_t *request_frame;

    printf("RECEIVED MESSAGE FROM BROKER\n");


    puts("REQUEST BODY FRAMES");
    printf("NUMBER OF FRAMES: %d\n", size_request);

    char *s; //string extracted from each frame


    long start = zclock_usecs(); // time to start popping requests
    // extract frames "for loop"

    for (int i = 0; i < size_request; i++) {
        //create a list of frames and for each one extracts the string
        request_frame = zmsg_next(request);
        long end = zclock_usecs();
        printf("TIME TO POP ONE SINGLE REQUEST: %ld [micro secs] \n", end - start);

        //duplicating string of the frame into the printable string
        s = zframe_strdup(request_frame);
        printf("BODY FRAME[%d]: %s\n", i, s);
        free(s);
        printf("FRAME TYPE-CONTENT: %s\n", "PLAIN STRING");
        printf("(BYTE) SIZE FRAME: %lu\n", zframe_size(request_frame));
        zframe_destroy(&request_frame);
    }

    long start_time_sending_reply;
    long end_time_sending_reply;

    start_time_sending_reply = zclock_usecs();
    //this is reply message initialization
    zmsg_t *reply_message = zmsg_new();
    //return the reply
    reply_message = handle_type_request(request);


    //send to broker the reply if exists
    mdp_worker_send(session, &reply_message, reply_to);
    end_time_sending_reply = zclock_usecs();
    printf("TIME TO SEND A REPLY: %ld\n", end_time_sending_reply - start_time_sending_reply);
    zframe_destroy(&reply_to);
    zmsg_destroy(&reply_message);
    zframe_destroy(&request_frame);

    return NULL;
}
