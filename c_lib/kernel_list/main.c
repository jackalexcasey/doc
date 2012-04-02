/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include "list.h"

struct hp_desc{
	struct list_head list;
	int a;
	int b;
};

struct hp_list{
	struct list_head p_list;
	struct list_head r_list;
};
static struct hp_list *ctrl=NULL;
static pthread_mutex_t hp_lock = PTHREAD_MUTEX_INITIALIZER;

void probe(int a, int b)
{
	struct hp_desc *p,*r;

	p = malloc(sizeof(struct hp_desc));
	if(!p)
		return -1;
	p->a = a;
	p->b = b;
	list_add(&p->list, &ctrl->p_list);
	list_for_each_entry(r, &ctrl->r_list, list){
		if((r->a == p->a) && (r->b == p->b)){
			fprintf(stderr,"Probe Match %d %d\n",r->a,r->b);
		}
	}
}

void release(int a, int b)
{
	struct hp_desc *p,r,*n;
	r.a = a;
	r.b = b;

	//list_for_each_entry_safe(r, n, &ctrl->p_list, list){
	list_for_each_entry(p, &ctrl->p_list, list){
		if((p->a == r.a) && (p->b == r.b)){
			fprintf(stderr,"Remove Match %d %d\n",p->a,p->b);
			list_del(&p->list);
		//	free(p);
			break;
		}
	}
}

void hpregister(int a, int b)
{
	struct hp_desc *r,*p;

	r = malloc(sizeof(struct hp_desc));
	if(!r)
		return -1;
	r->a = a;
	r->b = b;
	list_add(&r->list, &ctrl->r_list);
	list_for_each_entry(p, &ctrl->p_list, list){
		if((p->a == r->a) && (p->b == r->b)){
			fprintf(stderr,"Register Match %d %d\n",r->a, r->b);
		}
	}

}

void main(void)
{
	ctrl = malloc(sizeof(struct hp_list));
	if(!ctrl)
		return;

	INIT_LIST_HEAD(&ctrl->p_list);
	INIT_LIST_HEAD(&ctrl->r_list);

	probe(1,2);
	probe(2,2);
	probe(3,2);
	probe(4,2);
	probe(5,2);

	release(1,2);
	release(2,2);
	release(3,2);
	release(4,2);
	release(5,2);

	hpregister(1,2);
	hpregister(2,2);
	hpregister(3,2);
	hpregister(4,2);
	hpregister(5,2);

/*
	hpregister(1,0);
	hpregister(1,2);
	hpregister(1,3);
	hpregister(1,4);
	hpregister(1,5);
	hpregister(1,6);

	probe(1,2);
	probe(2,2);
	probe(3,2);
	probe(4,2);
	probe(5,2);

	hpregister(1,2);
	hpregister(2,2);
	hpregister(3,2);

	release(4,2);
	release(2,2);
	hpregister(4,2);
*/
}
