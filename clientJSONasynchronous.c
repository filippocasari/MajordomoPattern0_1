//
// Created by filippo on 01/04/2021.
// ASYNCHRONOUS CLIENT WITH JSON FORMAT PAYLOAD
//
#include <json-c/json.h>
#include <mdp.h>

#include "include/coffeeType.h"
// raspberry endpoint : "tcp://192.168.0.113:5000"
//localhost : "tcp://127.0.0.1:5000"
#define BROKER_ENDPOINT  "tcp://192.168.0.113:5000"
#define REQUEST "GET"

#define NUM_OF_REQUEST 10000

#define TYPE_REQUEST 0 //kind of coffee you want to require

int calculating_delay(const long *timestamps_receiving, const long *timestamps_sent, int const *count);

long calculating_time_serialization(json_object *REQ);


void print_serialized_object(struct json_object *REQ);

long create_JSON_object(struct json_object *REQ);


void calculating_average_time_serialization(const long *time_serialization_array);

int main(int argc, char *argv[]) {

    long time_serialization_array[NUM_OF_REQUEST];
    json_object *REQ = json_object_new_object(); //create a new object JSON


    //SAME CODE OF CLIENT ASYNCHRONOUS


    mdp_client_t *session2 = mdp_client_new(BROKER_ENDPOINT, 1);


    int count; // number of request
    zmsg_t *reply = NULL; //reply client will receive
    zmsg_t *request = zmsg_new(); // request initialization
    int64_t start; //start time to see processing time
    int64_t end; //end time


    //start the time

    start = zclock_mono();
    // send all request without wait the reply==> ZMQ doc calls this Asynchronous Client
    long timestamps_receiving[NUM_OF_REQUEST];
    long timestamps_sent[NUM_OF_REQUEST];


    for (count = 0; count < NUM_OF_REQUEST; count++) {

        time_serialization_array[count] = create_JSON_object(REQ); //returns a time and create and serializes
        const char *string_request = json_object_to_json_string(REQ); //converting to a string
        printf("\nSTRING REQUEST: %s\n", string_request);

        int succ = zmsg_pushstr(request, string_request); //push the string set before into the request message
        // handle error
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            break;
        }
        if (succ == -1) {
            puts("ERROR ");
        }
        //send request to broker for service "engine_1" in this case
        mdp_client_send(session2, "engine_1", &request);


        // reinitialize request
        request = zmsg_new();
        REQ = json_object_new_object();
    }

    //dealloc any msg or string

    zmsg_destroy(&request);
    zmsg_destroy(&reply);


    int num_no_replies = 0;
    int count_rep = 0;
    //for loop to receive reply messages
    for (; count_rep < NUM_OF_REQUEST; count_rep++) {
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            break;
        }
        char *command; //command received
        char *service; // from which service
        zmsg_t *reply2 = mdp_client_recv(session2, &command, &service); //reply if any
        timestamps_receiving[count_rep] = zclock_time();


        if (reply2 == NULL) {
            puts("NO REPLY...");
            num_no_replies++;
            continue;
        }
        char *reply_string = zmsg_popstr(reply2);
        json_object *REP;
        REP = json_tokener_parse(reply_string);
        puts("REPLY = ");
        long time_of_sending;
        json_object_object_foreach(REP, key2, val2) {
            printf("\t%s: %s\n", key2, json_object_to_json_string(val2));
            char *string_value = strdup(json_object_to_json_string(val2));
            char *key_str = strdup(key2);
            char *ptr;
            puts("");
            if (strcmp(key_str, "timestamp") == 0) {
                time_of_sending = strtol(string_value, &ptr, 10);
                timestamps_sent[count_rep] = time_of_sending;
                printf("Timestamp of captured receiving pack: %ld\n", timestamps_receiving[count_rep]);

                long time_end_to_end = timestamps_receiving[count_rep] - time_of_sending;

                printf("Time of delay end to end : \t%ld\n", time_end_to_end);

            }

        }
        //if reply is null, just tell to stdout

        zmsg_destroy(&reply2);
    }
    // end time
    end = zclock_mono() - start;

    //print how many requests client tried to send and how much time has just spent on it

    printf("%d requests processed\n", count);
    //printf("%d number of replies \n", count_rep);
    printf("%d number of received replies \n", count_rep - num_no_replies);
    printf("Time for Asynchronous Client is : %ld ms\n", end);
    puts("\n--------------------------------------------------------");


    //calculating average time of end to end delay

    calculating_delay(timestamps_receiving, timestamps_sent, &count);
    calculating_average_time_serialization(time_serialization_array);
    mdp_client_destroy(&session2); //destroy

    return 0;
}

void calculating_average_time_serialization(const long *time_serialization_array) {
    long sum = 0;
    for (int i = 0; i < NUM_OF_REQUEST; i++) {
        sum += time_serialization_array[i];
    }
    long double serialization_average_time =(long double) sum / NUM_OF_REQUEST;
    printf("AVERAGE TIME OF SERIALIZATION: %Lf [micro seconds]", serialization_average_time);
}

long create_JSON_object(struct json_object *REQ) {

    long time_serialization = calculating_time_serialization(REQ);
    if (time_serialization == -1) {
        fprintf(stderr, "Error while calculation time serialization");
    }

    printf("\nTime for serialization is: %ld\n", time_serialization);

    print_serialized_object(REQ);
    return time_serialization;

}

void print_serialized_object(json_object *REQ) {
    //to print what is inside the obj
    puts("JSON REQUEST: ");

    json_object_object_foreach(REQ, key, val) {
        printf("\t%s: %s\n", key, json_object_to_json_string(val));
    }
}

int calculating_delay(const long *timestamps_receiving, const long *timestamps_sent, const int *count) {
    int sum = 0;
    for (int i = 0; i < *count; i++) {
        sum += (int) (timestamps_receiving[i] - timestamps_sent[i]);
    }
    double average = (double) sum / *count;


    if (average < 0) {
        puts("average negative...impossible, exit");
        return 1;
    }
    printf("AVERAGE TIME END TO END: %f [ms]\n", average);

    return 0;
}

long calculating_time_serialization(struct json_object *REQ) {
    long time;
    long end;
    long start = zclock_usecs();


    //following lines are about adding new string to json obj
    json_object_object_add(REQ, "TYPE", json_object_new_string("REQ"));
    json_object_object_add(REQ, "REQ_TYPE", json_object_new_string(REQUEST));
    json_object_object_add(REQ, "SENSOR", json_object_new_string("SPEED"));
    end = zclock_usecs();
    time =(end - start);
    if (time < 0) {
        return -1;
    }
    printf("Time for serialization %ld [micro sec]", time);
    return time;
}
