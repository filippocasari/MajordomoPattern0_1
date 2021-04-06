//
// Created by utente on 01/04/2021.
//
#include <json-c/json.h>
#include <mdp.h>

#include "include/coffeeType.h"
// raspberry endpoint : "tcp://192.168.0.113:5000"
//localhost : "tcp://127.0.0.1:5000"
#define BROKER_ENDPOINT  "tcp://127.0.0.1:5000"
#define REQUEST "GET"

#define TYPE_REQUEST 0 //kind of coffee you want to require


int main(int argc, char *argv[]) {


    json_object *REQ=json_object_new_object();
    json_object_object_add(REQ, "type", json_object_new_string("REQ"));
    json_object_object_add(REQ, "REQ_TYPE", json_object_new_string(REQUEST));
    json_object_object_add(REQ, "SENSOR", json_object_new_string("SPEED"));
    puts("JSON REQUEST: ");
    json_object_object_foreach(REQ, key, val)
    {
        printf("\t%s: %s\n", key, json_object_to_json_string(val));
    }
    const char *string_request=json_object_to_json_string(REQ);
    printf("\nSTRING REQUEST: %s", string_request);


    mdp_client_t *session2 = mdp_client_new(BROKER_ENDPOINT, 1);


    int count; // number of request
    zmsg_t *reply = NULL; //reply client will receive
    zmsg_t *request = zmsg_new(); // request initialization
    int64_t start; //start time to see processing time
    int64_t end; //end time


    //start the time

    start= zclock_mono();
    // send all request without wait the reply==> ZMQ doc calls this Asynchronous Client

    for (count = 0; count < 50; count++) {

        int succ = zmsg_pushstr(request, string_request); //push the string set before into the request message
        // handle error
        if (succ == -1) {
            puts("ERROR ");
        }
        //send request to broker for service "coffee" in this case
        mdp_client_send(session2, "engine_1", &request);
        // reinitialize request
        request = zmsg_new();
    }

    //dealloc any msg or string

    zmsg_destroy(&request);
    zmsg_destroy(&reply);


    int num_no_replies=0;
    int count_rep=0;
    //for loop to receive reply messages
    for (; count_rep < 50; count_rep++) {
        if (zctx_interrupted) {
            zclock_log("error signal handled...");
            break;
        }
        char *command; //command received
        char *service; // from which service
        zmsg_t *reply2 = mdp_client_recv(session2,&command, &service); //reply if any
        if(reply2==NULL){
            puts("NO REPLY...");
            num_no_replies++;
            continue;
        }
        char *reply_string=zmsg_popstr(reply2);
        json_object *REP;
        REP=json_tokener_parse(reply_string);
        puts("REPLY = ");
        json_object_object_foreach(REP, key, val){
            printf("\t%s: %s\n", key, json_object_to_json_string(val));

        }
        //if reply is null, just tell to stdout

        zmsg_destroy(&reply2);
    }
    // end time
    end = zclock_mono() - start;

    //print how many requests client tried to send and how much time has just spent on it

    printf("%d requests processed\n", count);
    //printf("%d number of replies \n", count_rep);
    printf("%d number of received replies \n", count_rep-num_no_replies);
    printf("Time for Asynchronous Client is : %ld ms", end);
    mdp_client_destroy(&session2); //destro

    return 0;
}




