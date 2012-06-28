#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>

unsigned *p = NULL;

void segv_handler(int sig, siginfo_t *inf, void *ctx)
{
	// Buffer is RW.
	if(mprotect(p, 1024, PROT_READ|PROT_WRITE))
	{
	perror("segv_handler: couldn't mprotect");
	exit(errno);
	}

#if 0
	if((unsigned*)inf->si_addr >= p // Fault address within
		&& (unsigned*)inf->si_addr < p + 1024) // the correct range.
	{
	// Buffer is RW.
	if(mprotect(p, 1024, PROT_READ|PROT_WRITE))
	{
	perror("segv_handler: couldn't mprotect");
	exit(errno);
	}
	// Do some other stuff...
	}
#endif
	printf("HANDLER\n\n\n");
}

int main()
{
// Install handler
struct sigaction sa;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_SIGINFO;
sa.sa_sigaction = &segv_handler;
sigaction(SIGSEGV, &sa, NULL);

// Allocate buffer
p = (unsigned*)mmap(0x12345678, 1024, PROT_READ|PROT_WRITE,
MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

unsigned x = p[500]; // Read; ok
p[0] = 42; // Write; ok

/*
// Buffer is RO.
if(mprotect(p, 1024, PROT_READ))
{
perror("main: couldn't mprotect");
exit(errno);
}
*/

x = p[0]; // Read; ok
p[0] = 200; // Write; segv_handler called
printf("%u\n",p[0]);

return 0;
}
