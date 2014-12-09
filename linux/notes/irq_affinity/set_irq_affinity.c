#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <linux/limits.h>

int set_irq_affinity(char* filter, char *cpu_mask)
{
	FILE *file, *file2;
	char *irq_name, *savedptr, *last_token, *p;
	char buf[PATH_MAX];
	char *line = NULL;
	size_t size = 0;

	file = fopen("/proc/interrupts", "r");
	if (!file)
		return -1;

	/* first line is the header we don't need; nuke it */
	if (getline(&line, &size, file)==0) {
		free(line);
		fclose(file);
		return -1;
	}

	while (!feof(file)) {
		int	 number;
		char *c;
		char savedline[1024];

		if (getline(&line, &size, file)==0)
			break;

		/* lines with letters in front are special, like NMI count. Ignore */
		c = line;
		while (isblank(*(c)))
			c++;
			
		if (!(*c>='0' && *c<='9'))
			break;
		c = strchr(line, ':');
		if (!c)
			continue;

		strncpy(savedline, line, sizeof(savedline));

		irq_name = strtok_r(savedline, " ", &savedptr);
		last_token = strtok_r(NULL, " ", &savedptr);
		while ((p = strtok_r(NULL, " ", &savedptr))) {
			irq_name = last_token;
			last_token = p;
		}

		*c = 0;
		c++;
		number = strtoul(line, NULL, 10);

		if (strstr(last_token, filter) != NULL) {
			sprintf(buf, "/proc/irq/%i/smp_affinity", number);
			file2 = fopen(buf, "w");
			if (!file2)
				return -1;
			fprintf(file2, "%s", cpu_mask);
			fclose(file2);
			fprintf(stderr,"%s -> %s\n",buf, cpu_mask);
		}
	}
	fclose(file);
	free(line);
	return 0;
}

/*
 * EX: Set all irq multicast to affinity mask 2 ( vCPU1 )
 * irq_affinity multicast 2
 */
int main(int argc, char*argv[])
{
	int err;

	if(argc != 3)
		return -1;
	
	err = set_irq_affinity(argv[1], argv[2]);
	if(err)
		fprintf(stderr,"Cannot set %s %s\n",argv[1], argv[2]);
	else
		fprintf(stderr,"done\n");
}


