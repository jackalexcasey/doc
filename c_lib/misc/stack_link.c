/* Last in First out */

#include <stdlib.h>

struct stack_entry{
	struct stack_entry *prev;
	int value;
};


struct stack_entry * push(struct stack_entry *stack, int value)
{
	struct stack_entry *ptr;
	
	ptr=malloc(sizeof(struct stack_entry));
	ptr->value = value;
	ptr->prev=stack;
	return ptr;
}

struct stack_entry * pop(struct stack_entry *stack, int *val)
{
	struct stack_entry *ptr;

	*val = stack->value;
	ptr = stack->prev;
	free(stack);
	return ptr;
}

int full(int *sp, int size)
{
//	return (*sp == size ? 1:0);
}

int empty(struct stack_entry *stack)
{
	return (stack->prev == NULL ? 1 :0);
}

struct stack_entry * init(void)
{
	struct stack_entry *ptr;
	
	ptr=malloc(sizeof(struct stack_entry));
	ptr->prev=NULL;
	return ptr;
}

#define STACK_SIZE 1024
void main(void)
{
	int x,val;
	struct stack_entry *stack = init();

	for(x=0;x<100;x++){
		stack = push(stack, x);
	}

	while(!empty(stack)){
		stack = pop(stack, &val);
		printf("Pop %d \n",val);
	}

}


#if 0

#include<stdlib.h>

struct node{
	int data;
	struct node* next;
};

void init(struct node* s){
	s = NULL;
}

struct node* push(struct node* s,int data)
{
	struct node* tmp = (struct node*)malloc(sizeof(struct node));
	if(tmp == NULL){
		// no memory available
		exit(0);
	}
	tmp->data = data;
	tmp->next = s;
	s = tmp;
	return s;
}
struct node* pop(struct node *s,int *element)
{
	struct node* tmp = s;
	*element = s->data;
	s = s->next;
	free(tmp);
	return s;
}

int empty(struct node* s){
	return s == NULL ? 1 : 0;
}

#include <stdio.h>
#include <stdlib.h>



void main()
{
	struct node* head = NULL;
	int size,element,counter = 0;

	/* 
		 stack size is dynamic and 
	  	 specified at runtime 
	*/
	printf("Enter stack size:");
	scanf("%d",&size);

	printf("Push elements to stack\n");
	init(head);
	while(counter < size)
	{
		 getchar();
         element = rand();
		 printf("push element %d into stack\n",element);
         head = push(head,element);
		 counter++;
    }
	printf("Pop elements from stack\n");
	while(0 == empty(head))
	{
		head = pop(head,&element);
		printf("pop element %d from stack\n",element);
		getchar();
	}

	getchar();
}

#endif

