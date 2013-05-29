       #define _GNU_SOURCE
       #include <stdio.h>
       #include <stdlib.h>

       int
       main(int argc, char *argv[])
       {
           FILE * fp;
           char * line = NULL;
           size_t len = 0;
           ssize_t read;
		   int range,value,total=0,low=0,max;
			unsigned long avg=0;

           fp = fopen(argv[1], "r");
           if (fp == NULL)
               exit(EXIT_FAILURE);

           while ((read = getline(&line, &len, fp)) != -1) {
		   		sscanf(line,"%d %d", &range, &value);
				avg = avg +(range*value);
				if(range >max)
					max=range;
				if(range >=1000)
					total += value;
				else
					low += value;
           }

           if (line)
               free(line);
		   printf("\n[>1ms = %d] [<1ms = %d] [Sum = %d] [Max = %d uSec] [Avg = %d uSec]\n",total,low,total+low,max,avg/(total+low));
           exit(EXIT_SUCCESS);
       }
