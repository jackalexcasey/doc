struct packed_struct {
	unsigned int f1:1;
	unsigned int f2:1;
	unsigned int f3:1;
	unsigned int f4:1;
};

void main(void)
{
	struct packed_struct t;

	printf("\n %d",sizeof(t));

	t.f1=1;
}
