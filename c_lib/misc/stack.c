/* Last in First out */



void push(int *stack, int *sp, int val)
{
	 stack[(*sp)++]=val;
	// OR
//	 stack[*sp] = val;
//	 (*sp)++;
}

int pop(int *stack, int *sp)
{
	return stack[--(*sp)];

}

int full(int *sp, int size)
{
	return (*sp == size ? 1:0);
}

int empty(int *sp)
{
	return (*sp == 0 ? 1:0);
}

#define STACK_SIZE 1024
void main(void)
{
	int sp,x;
	int stack[STACK_SIZE];

	sp=0;

	for(x=0;x<100;x++){
		push(stack,&sp,x);
	}

	for(x=0;x<100;x++){
		printf("Pop %d\n",pop(stack,&sp));
	}

}

