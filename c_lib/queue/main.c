#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define QUEUE_SIZE 32

struct data{
	int val;
};

struct queue{
	int head;
	int tail;
	struct data d[QUEUE_SIZE];
};

static struct queue q = {
	.head = QUEUE_SIZE-1,
	.tail = QUEUE_SIZE-1,
};

int empty(struct queue *q)
{
	return (q->head == q->tail ? 1 : 0);
}

int full(struct queue *q)
{
	int t;

	t = q->head;
	if(t == QUEUE_SIZE-1)
		t = 0;
	else
		t++;
	if(t == q->tail)
		return 1;
	return 0;
}
	
int insert(struct queue *q, struct data *d)
{
	if(full(q))
		return -1;
	if(q->head == QUEUE_SIZE -1)
		q->head = 0;
	else
		q->head++;
	memcpy(&q->d[q->head],d, sizeof(struct data));
	return 0;
}

int fetch(struct queue *q, struct data *d)
{
	if(empty(q))
		return -1;
	if(q->tail == QUEUE_SIZE -1)
		q->tail = 0;
	else
		q->tail++;
	memcpy(d, &q->d[q->tail], sizeof(struct data));
	return 0;
}

int main(void)
{
	int x;
	struct data data;

	for (x=0;x<1024;x++){
		data.val = x;
		insert(&q,&data);
	}

	for(x=0;x<10;x++){
		if(!fetch(&q,&data))
			printf("%d\n",data.val);
	}
	for (x=100;x<1024;x++){
		data.val = x;
		insert(&q,&data);
	}
	for(x=0;x<1024;x++){
		if(!fetch(&q,&data))
			printf("%d\n",data.val);
	}
	return 0;
}
