#include <stdio.h>

#define PATCH_EXPORT_SYMBOL(sym)					\
	PATCH__EXPORT_SYMBOL(sym, "")

#define NEW(ticker,date_day,date_month,date_year,price,curr,broker) \
	void ticker (void) __attribute__ ((section ("bar"))); \
	void ticker (void) {\
		printf("Position %s\n",#ticker); \
	}

const char* array __attribute__((section("test"))) = "etienne";

#include "position.h"

extern const int bar;
extern const int test;

void main(void)
{
	MSFT();
	printf("%x %s\n",bar,test);
}

