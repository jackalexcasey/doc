#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct data{
	int val;
};

struct list{
	struct list *next;
	struct data d;
};

static struct list l = {
	.next = NULL,
};

int list_add(struct list *l, struct data *d)
{
	struct list *t;

	if(!l)
		return -1;

	t = malloc(sizeof(struct list));
	if(!t)
		return -1;
	
	memcpy(&t->d,d,sizeof(struct data));
	t->next = l->next;
	l->next = t;
	return 0;
}

int list_del(struct list *l)
{
	struct list *t;

	if(l->next == NULL)
		return -1;

	t = l->next;
	l->next = t->next;
	free(t);
	return 0;
}

struct list * list_search(struct list *l, struct data *d)
{
	struct list *t;

	t=l;
	while(t != NULL){
		if(memcmp(&t->d, d, sizeof(struct data))==0)
			return t;
		t = t->next;
	}

	return NULL;
}

int main(void)
{
	int x;
	struct data data;
	struct list *t;

	for (x=0;x<10;x++){
		data.val = x;
		list_add(&l, &data);
		printf("added %d\n",x);
	}
	
	for (x=0;x<10;x++){
		data.val = x;
		t = list_search(&l, &data);
		if(!t)
			printf("removed %d\n",x);
		else
			printf("found %d\n",x);
	}
	printf("\n");

	for (x=0;x<10;x++){
		data.val = x;
		t = list_search(&l, &data);
		if(!t)
			printf("removed %d\n",x);
		else
			printf("found %d\n",x);
		if(x==2 || x==5){
			printf("del %d\n",t->next->d.val);
			list_del(t);
		}
	}
	printf("\n");
	
	for (x=0;x<20;x++){
		data.val = x;
		t = list_search(&l, &data);
		if(!t)
			printf("removed %d\n",x);
		else
			printf("found %d\n",x);
	}
	printf("\n");

	return 0;
}
