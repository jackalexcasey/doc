#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

typedef uint32_t uatomic32_t;


#define LOCK_PREFIX "lock;"
# define _cmpxchgl_(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile (LOCK_PREFIX "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

int va;
int vb;
int vc;
int vd;
int ve;
int vf;

uint32_t lock=0;
void atomic_add(int *val, int incr)
{
	while (_cmpxchgl_(&lock,1,0)){} /* custom spinlock */
	*val=*val+incr;
	lock=0;
}

#define TEST_FCN(name) \
static void* t##name(void* arg) \
{ \
	int x; \
	for(x=0;x<50000;x++){ \
		atomic_add(&va,1); \
		atomic_add(&vb,1); \
		atomic_add(&vc,1); \
		atomic_add(&vd,1); \
		atomic_add(&ve,1); \
		atomic_add(&vf,1); \
	} \
	return NULL;\
}

TEST_FCN(1)
TEST_FCN(2)
TEST_FCN(3)

void main(void)
{
	pthread_t a,b,c,d,e;
	
	va=0;
	vb=0;
	vc=0;
	vd=0;
	ve=0;
	vf=0;

	pthread_create(&a, NULL, &t1, NULL);
	pthread_create(&b, NULL, &t2, NULL);
	pthread_create(&c, NULL, &t3, NULL);
	
	pthread_join(a,NULL);
	pthread_join(b,NULL);
	pthread_join(c,NULL);
	printf("\n %d %d %d %d %d %d",va,vb,vc,vd,ve,vf);

}

