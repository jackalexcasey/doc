void main(void)
{
	int a = 0x55;
	printf("\n %x",a);

	a = a & 0xff;
	printf("\n %x",a);

	printf("\n %x",~a);


	a = a & (~0x1);
	printf("\n %x",a);

}
