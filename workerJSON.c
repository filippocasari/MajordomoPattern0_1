

#include <mdp.h>

#include <json-c/json.h>

#define NUM_WORKERS 1
// "tcp://192.168.0.113:5000"
// "tcp://127.0.0.1:5000"
//"tcp://192.168.1.7:5000"
#define BROKER_ENDPOINT  "tcp://192.168.1.7:5000"

#define VENDOR "Renault"
#define POWER "90cv"


static void workerTask(zsock_t *pipe, void *args);

zmsg_t *handle_type_request(zframe_t *request[]);

void print_parsing_time(const long *start, const long *end);

void print_average_parsing_time(const long *parsing_array, const size_t *num_samples);

int check_option(const int *argc, char *argv[], int *daemonize, int *verbose, int *user_endpoint);

double speed = 130.0;

int main(int argc, char *argv[]) {
    int verbose = 0;
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

    //TODO managing of arguments!!!!!
    zsys_catch_interrupts();
    int *arguments = (int *) args;
    int verbose = arguments[0];


    //set if you wanna get logger



    //create a new worker, you have to pass "endpoint of the broker", "service required" and the verbose
    long end;

    long time_to_signup;
    long time_to_close_connection;
    long *times_of_pop_request;
    char *endpoint= BROKER_ENDPOINT;
    long start = zclock_usecs();
    mdp_worker_t *session = mdp_worker_new(
            endpoint, "engine_1", verbose);
    end = zclock_usecs();
    time_to_signup = end - start;
    printf("TIME TO SIGN UP: %ld [micro secs]\n", time_to_signup);
    mdp_worker_set_heartbeat(session,
                             7500); //set the heartbeat time. After this time in seconds, worker will send to worker an heartbeat message


    zframe_t *reply_to; // this frame identifies from who (which client) the message is
    zmsg_t *reply_message; // create a new message for reply



    //until exception
    int count = 0;
    srand(time(NULL));
    while (1) {

        if (zctx_interrupted || zsys_interrupted) {
            zclock_log("error signal handled...");
            start = zclock_usecs();
            mdp_worker_destroy(&session);
            end = zclock_usecs();
            time_to_close_connection = end - start;
            printf("TIME TO CLOSE CONNECTION: %ld [micro secs]\n", time_to_close_connection);
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


        int size_request = (int) zmsg_size(request); //SIZE of the request
        times_of_pop_request = (long *) malloc(size_request * sizeof(long));
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


        start = zclock_usecs(); // time to start popping requests
        // extract frames "for loop"

        for (int i = 0; i < size_request; i++) {
            //create a list of frames and for each one extracts the string
            request_stream[i] = zmsg_pop(request);
            end = zclock_usecs();
            printf("TIME TO POP ONE SINGLE REQUEST: %ld [micro secs] \n", end - start);
            times_of_pop_request[i] = end - start;
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
        for (int i = 0; i < size_request; i++) {
            sum += times_of_pop_request[i];
        }
        average_time_pop_request = (long double) sum / size_request;
        puts("---------------------------------------------------");
        printf("AVERAGE TIME TO POP REQUEST: %Lf [micro secs]\n", average_time_pop_request);
        free(times_of_pop_request);
        start_time_sending_reply=zclock_usecs();
        //this is reply message initialization
        reply_message = zmsg_new();
        //return the reply
        reply_message = handle_type_request(request_stream);


        //send to broker the reply if exists
        mdp_worker_send(session, &reply_message, reply_to);
        end_time_sending_reply=zclock_usecs();
        printf("TIME TO SEND A REPLY: %ld\n", end_time_sending_reply-start_time_sending_reply;
    }
    mdp_worker_destroy(&session); //free instance

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
