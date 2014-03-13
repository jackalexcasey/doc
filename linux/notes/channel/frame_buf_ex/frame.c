#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/mman.h>

int main()
{
   struct fb_var_screeninfo screen_info;
   struct fb_fix_screeninfo fixed_info;
   char *buffer = NULL;
   size_t buflen;
   int fd = -1;
   int r = 1;
   int x;

   fd = open("/dev/fb0", O_RDWR);
   if (fd >= 0)
   {
      if (!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info) &&
          !ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info))
      {
         buflen = screen_info.yres_virtual * fixed_info.line_length;
         buffer = mmap(NULL,
                       buflen,
                       PROT_READ|PROT_WRITE,
                       MAP_SHARED,
                       fd,
                       0);
         if (buffer != MAP_FAILED)
         {
            /*
             * TODO: something interesting here.
             * "buffer" now points to screen pixels.
             * Each individual pixel might be at:
             *    buffer + x * screen_info.bits_per_pixel/8
             *           + y * fixed_info.line_length
             * Then you can write pixels at locations such as that.
             */
			fprintf(stderr, "INFO %d %d %d\n",screen_info.xres_virtual, screen_info.yres_virtual,
				screen_info.bits_per_pixel);
			for(x=0;x<1000;x++){
				buffer[x] = 0x55;
			}

             r = 0;   /* Indicate success */
         }
         else
         {
            perror("mmap");
         }
      }
      else
      {
         perror("ioctl");
      }
   }
   else
   {
      perror("open");
   }

   /*
    * Clean up
    */
   if (buffer && buffer != MAP_FAILED)
      munmap(buffer, buflen);
   if (fd >= 0)
      close(fd);

   return r;
}
