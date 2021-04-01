#include <mdp.h>
#include "include/workerTask.h"


#define NUM_WORKERS 1



int main(){

    zactor_t *workers[NUM_WORKERS];
    for(int i=0; i<NUM_WORKERS; i++){
        workers[i]=zactor_new(workerTask, NULL);
    }
    zsys_catch_interrupts();
    for(int i=0; i<NUM_WORKERS; i++){
        zactor_destroy(&workers[i]);
    }




    return 0;
}

