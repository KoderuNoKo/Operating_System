#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
	if (q->size == MAX_QUEUE_SIZE) {
		return;
	}
        q->proc[q->size++] = proc;
}

struct pcb_t * dequeue(struct queue_t * q) {
	if(empty(q)) return NULL;
	if(q -> size == 1)
	{
		q -> size--;
		struct pcb_t * proc = q->proc[0];
		//printf("\tSlot : %d\n",q -> slot);
		//printf("\tSlot MAX: %d\n",q ->slotMax);
		//printf("\tSlot remaining: %d\n",q -> slotMax - q -> slot);
		return proc;
	}
	else
	{
	struct pcb_t * proc = q->proc[0];
	q -> size--;
		for(int i = 0; i < q -> size; i++)
		{
			q -> proc[i] = q -> proc[i+1];
		}
		//printf("\tSlot: %d\n",q -> slot);
		//printf("\tSlot MAX: %d\n",q ->slotMax);
		//printf("\tQueue size: %d\n",q -> size);
		//printf("\tSlot remaining: %d\n",q -> slotMax - q -> slot);
		return proc;
	}
	return NULL;
}