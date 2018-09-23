#ifndef QUEUE_H_   /* Include guard */
#define QUEUE_H_
	typedef struct Queue
	{
    	int front, rear, size;
    	unsigned capacity;
    	int* array;
	} Queue;
	Queue* createQueue(unsigned capacity);
	int isFull(struct Queue* queue);
	int isEmpty(struct Queue* queue);
	void enqueue(struct Queue* queue, int item);
	int dequeue(struct Queue* queue);
	int front(struct Queue* queue);
	int rear(struct Queue* queue);
#endif