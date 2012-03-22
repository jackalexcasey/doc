
struct a{
	int q;
};

struct a st_a;

struct test{
	int q;
}st_test;

typedef struct tt{
	int q;
}t_tt;

void main(void)
{
	struct a qq;
	t_tt w;

	qq.q=1;
	st_a.q=1;
	st_test.q=2;
	w.q=0;
	
}
