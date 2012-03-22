#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct data{
	int val;
};

struct tree{
	struct tree *left;
	struct tree *right;
	struct data d;
};

struct tree * alloc_tree(struct data *d)
{
	struct tree *t;
	t = malloc(sizeof(struct tree));
	if(!t)
		return NULL;
	t->left = NULL;
	t->right = NULL;
	memcpy(&t->d,d,sizeof(struct data));
	return t;
}

struct tree* left(struct tree* t)
{
	return t->left;
}

struct tree* right(struct tree* t)
{
	return t->right;
}

int set_left(struct tree* t, struct data *d)
{
	if(!t)
		return -1;
	t->left = alloc_tree(d);
	if(!t->left)
		return -1;
	return 0;
}

int set_right(struct tree *t, struct data *d)
{
	if(!t)
		return -1;
	t->right = alloc_tree(d);
	if(!t->right)
		return -1;
	return 0;
}

/* left, visit root, right */
void inorder_trav(struct tree *t)
{
	if(!t)
		return;
	inorder_trav(t->left);
	printf("%d\n",t->d.val);
	inorder_trav(t->right);
}

int value[] = {14,15,4,9,7,18,3,5,16,20,17,9,5,0};

int main(void)
{
	int x;
	struct data d;
	struct tree *t,*p,*q;

	d.val = value[0];
	t = alloc_tree(&d); //root node
	
	x=1;
	while(value[x]!=0){
		d.val = value[x];

		p = t;

		/* get to the leaf */
		while(p!=NULL){
			q = p; //Remember the last valid p
			if(d.val > p->d.val)
				p = right(p);
			else
				p = left(p);
		}
		if(d.val > q->d.val)
			set_right(q,&d);
		else
			set_left(q,&d);

		x++;
	}

	inorder_trav(t);

	return 0;
}
