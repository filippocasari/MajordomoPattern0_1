//
// Created by utente on 27/04/2021.
//

#ifndef MAJORDOMOPATTERN0_1_QUEUE_H
#define MAJORDOMOPATTERN0_1_QUEUE_H


struct Queue;

extern struct Queue*
newQueue(int capacity);

extern int
enqueue(struct Queue *q, zmsg_t *value);

extern void*
dequeue(struct Queue *q);

extern void
freeQueue(struct Queue *q);


#endif //MAJORDOMOPATTERN0_1_QUEUE_H
