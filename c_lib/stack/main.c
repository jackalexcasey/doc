#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STACK_SIZE 256

struct data{
	int val;
};

struct stack{
	int top;
	struct data d[STACK_SIZE];
};

static struct stack s = {
	.top = -1,
};


int empty(struct stack *s)
{
	return (s->top == -1 ? 1 : 0);
}

int full (struct stack *s)
{
	return (s->top == STACK_SIZE -1 ? 1 : 0);
}

int push (struct stack *s, struct data *d)
{
	if(full(s))
		return -1;
	s->top++;
	memcpy(&s->d[s->top],d, sizeof(struct data));
	return 0;
}

int pop (struct stack *s, struct data *d)
{
	if(empty(s))
		return -1;
	memcpy(d, &s->d[s->top], sizeof (struct data));
	s->top--;
	return 0;
}

int main(void)
{
	int x;
	struct data data;

	for (x=0;x<1024;x++){
		data.val = x;
		push(&s,&data);
	}

	for(x=0;x<1024;x++){
		if(!pop(&s,&data))
			printf("%d\n",data.val);
	}
	return 0;
}
