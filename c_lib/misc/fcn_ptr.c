int test(int a)
{
	printf("%d\n",a);
	return a;
}

typedef int(*fcn_t)(int);

void main(void)
{
	fcn_t fptr;
	int (*fcn)(int);

	fptr = test;
	fcn = test;
	fptr(123);
	fcn(321);

}
