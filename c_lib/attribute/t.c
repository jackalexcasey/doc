#include <stdio.h>

#define NEW(ticker,date_day,date_month,date_year,price,curr,broker) \
	void ticker (void) __attribute__ ((section ("bar"))); \
	void ticker (void) {\
		printf("Position %s\n",#ticker); \
	}

struct data{
	char name[16];
};

struct data t1 __attribute__((section("test"))) = {"etienne"};
struct data t2 __attribute__((section("test"))) = {"etienne1"};
struct data t3 __attribute__((section("test"))) = {"etienne2"};

#include "position.h"

extern const int bar;
extern const int test;

void main(void)
{
	struct data *ptr;

	ptr = &test;

	MSFT();
	printf("%x %s\n",bar,ptr->name);
	ptr ++;
	printf("%x %s\n",bar,ptr->name);

}
