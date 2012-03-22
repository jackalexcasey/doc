//String operator takes value1 and does "value1"

#define TT(a,b) printf( #a "and" #b "\n")

int a1=0;
int a2=1;
int a3=2;

//Token operator takes literral
#define test(num) printf("\n %d",a##num)

char *test[3];

void main(void)
{
	TT(value1,value2);

	test(1);
	test(2);
	test(3);
}
