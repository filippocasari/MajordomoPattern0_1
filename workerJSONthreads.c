

#include <mdp.h>

#include <json-c/json.h>

#include "DynamicQueueFIFO/queue.h"


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

zmsg_t *handle_type_request(zframe_t *request[]);

void print_parsing_time(const long *start, const long *end);

void print_average_parsing_time(const long *parsing_array, const size_t *num_samples);

int check_option(const int *argc, char *argv[], int *daemonize, int *verbose, int *user_endpoint);


struct counter_t {
    int value;
    pthread_mutex_t lock;
};

static struct counter_t num_replies_consum;
static struct counter_t num_replies_prod;

void init(struct counter_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

void increment_by(struct counter_t *c, int by) {
    pthread_mutex_lock(&c->lock);
    c->value += by;
    pthread_mutex_unlock(&c->lock);
}

void increment(struct counter_t *c) {
    increment_by(c, 1);
}

int get(struct counter_t *c) {
    pthread_mutex_lock(&c->lock);
    int rc = c->value;
    pthread_mutex_unlock(&c->lock);
    return rc;
}


double speed = 130.0;

#define BUFFER_SIZE 100
void *buffer[BUFFER_SIZE];
Queue *queue ;
zframe_t *replies_to[50];


mdp_worker_t *session;

// Number of producer threads to start
// Must be greater than zero
#define NUM_OF_PRODUCERS 5
#define NUM_OF_CONSUMERS 5



struct thread_args {

    mdp_worker_t *session;
    zframe_t *reply_to;

};
struct thread_args args_of_thread;

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
}

static void
workerTask(zsock_t *pipe, void *args) {

    queue=queue_new(INF);
    init(&num_replies_consum);
    init(&num_replies_prod);

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
    pthread_t producers[NUM_OF_PRODUCERS];

    int num = 0;
    while (!zctx_interrupted) {


        zframe_t *reply_to;
        zmsg_t *request = mdp_worker_recv(session, &reply_to);
        //replies_to[NUM_OF_PRODUCERS % 50] = reply_to;


        pthread_create(&producers[num % NUM_OF_PRODUCERS], NULL, producer, &request);
        printf("Starting Thread producer %d\n", num%NUM_OF_PRODUCERS);
        pthread_create(&consumers[num % NUM_OF_CONSUMERS], NULL, consumer, &reply_to);
        printf("Starting Thread consumer %d\n", num%NUM_OF_CONSUMERS);
        pthread_join(producers[num % NUM_OF_PRODUCERS], NULL);

        pthread_join(consumers[num % NUM_OF_CONSUMERS], NULL);

        num++;


    }
    mdp_worker_destroy(&session);


}

zmsg_t *handle_type_request(zframe_t *request[]) {

    long start_time_parsing;
    long end_time_parsing;
    size_t n = (int) (sizeof(request) / sizeof(request[0]));
    zframe_t *frame_n;

    zmsg_t *reply = zmsg_new();

    json_object *REQ[n];
    json_object *REP[n];
    long parsing_array[n];

    for (int i = 0; i < n; i++) {
        frame_n = request[i];
        start_time_parsing = zclock_usecs(); // start time of the parsing

        char *request_string_json = zframe_strdup(frame_n); //copy frame as a string
        REQ[i] = json_tokener_parse(request_string_json); //adding new request to array REQ
        end_time_parsing = zclock_usecs();

        parsing_array[i] = end_time_parsing - start_time_parsing;

        print_parsing_time(&start_time_parsing, &end_time_parsing);

        zframe_destroy(&frame_n);

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
        print_average_parsing_time(parsing_array, &n);

    }

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
    zmsg_t *request = malloc(zmsg_size(arg));
    request=arg;



    queue_push(queue, request);
    zmsg_destroy(&request);


    return NULL;
}

void *consumer(void *arg) {
    int SIZE_QUEUE = queue_size(queue);


    if(SIZE_QUEUE>0){

        zframe_t *reply_to =malloc(zframe_size(arg));
        reply_to=arg;

        zmsg_t *request = queue_pop(queue);
        //zframe_t *reply_to = replies_to[num_replies_consum.value % 50];
        increment(&num_replies_consum);
        int size_request = (int) zmsg_size(request);

        zframe_t *request_stream[size_request];

        printf("RECEIVED MESSAGE FROM BROKER\n");


        puts("REQUEST BODY FRAMES");
        printf("NUMBER OF FRAMES: %d\n", size_request);

        char *s = NULL; //string extracted from each frame


        long start = zclock_usecs(); // time to start popping requests
        // extract frames "for loop"

        for (int i = 0; i < size_request; i++) {
            //create a list of frames and for each one extracts the string
            request_stream[i] = zmsg_pop(request);
            long end = zclock_usecs();
            printf("TIME TO POP ONE SINGLE REQUEST: %ld [micro secs] \n", end - start);

            //duplicating string of the frame into the printable string
            s = zframe_strdup(request_stream[i]);
            printf("BODY FRAME[%d]: %s\n", i, s);

            printf("FRAME TYPE-CONTENT: %s\n", "PLAIN STRING");
            printf("(BYTE) SIZE FRAME: %lu\n", zframe_size(request_stream[i]));
        }
        long sum = 0;
        long double average_time_pop_request;
        long start_time_sending_reply;
        long end_time_sending_reply;

        average_time_pop_request = (long double) sum / size_request;
        puts("---------------------------------------------------");
        printf("AVERAGE TIME TO POP REQUEST: %Lf [micro secs]\n", average_time_pop_request);

        start_time_sending_reply = zclock_usecs();
        //this is reply message initialization
        zmsg_t *reply_message = zmsg_new();
        //return the reply
        reply_message = handle_type_request(request_stream);


        //send to broker the reply if exists
        mdp_worker_send(session, &reply_message, reply_to);
        end_time_sending_reply = zclock_usecs();
        printf("TIME TO SEND A REPLY: %ld\n", end_time_sending_reply - start_time_sending_reply);
    }


    return NULL;
}