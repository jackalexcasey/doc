#include <unistd.h>
#include <stdio.h>


void fcn2(void)
{
	static int a2=1;
	fprintf(stderr,"%d\n",__LINE__);
	a2++;
}

void fcn1(void)
{
	fprintf(stderr,"%d\n",__LINE__);
	fcn2();
}


void main(void)
{
	fcn1();
	fcn2();
}

