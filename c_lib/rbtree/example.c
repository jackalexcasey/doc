#include <stdlib.h>
#include <stdio.h>
#include <linux/rbtree.h>

struct test {
	struct rb_node list;
	int value;
};

static struct rb_root *head;

static void rb_insert(struct test *n)
{
	struct rb_node **link = &head->rb_node;
	struct rb_node *parent = NULL;
	struct test *x;

	/*
	 * Find the right place in the rbtree:
	 */
	while (*link) {
		parent = *link;
		x = rb_entry(parent, struct test, list);

		if (n->value < x->value)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	/*
	 * Insert to the rbtree
	 */
	rb_link_node(&n->list, parent, link);
	rb_insert_color(&n->list, head);
}

static struct test * rb_search(int value)
{
	struct rb_node *n = head->rb_node;
	struct test * tmp;

	while (n)
	{
		tmp = rb_entry(n, struct test, list);

		if (value < tmp->value)
			n = n->rb_left;
		else if (value > tmp->value)
			n = n->rb_right;
		else
			return tmp;
	}
	return NULL;
}

void main(void)
{
	int x;
	struct test *t;
	struct rb_node *n;

	head = malloc(sizeof(struct rb_root));
	if (head == NULL) {
		perror("failed to allocate memory for device\n");
		exit(0);
	}

	for(x=0;x<100;x++){
		t = malloc(sizeof(struct test));
		if (t == NULL){
			perror("malloc failed \n");
			exit(0);
		}
		t->value = x;
		rb_insert(t);
	}

	n = rb_first(head);
	t = rb_entry(n, struct test, list);
	printf("\n %d",t->value);
	
	n = rb_last(head);
	t = rb_entry(n, struct test, list);
	printf("\n %d",t->value);

	n = rb_first(head);
	while(1){
		n = rb_next(n);
		if(!n)
			break;
		t = rb_entry(n, struct test, list);
		printf("\n %d",t->value);
	}

	t = rb_search(44);
	if(t)
		printf("\n %d",t->value);
	
	rb_erase(&t->list, head);
	t = rb_search(44);
	if(t)
		printf("\n %d",t->value);
}

