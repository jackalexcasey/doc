void main(void)
{
	int a=1;
	register b=2;

	printf("%d %d\n",a,b);
	
	printf("%p %p\n",&a,&b); //error: address of register variable ‘b’ requested

}
