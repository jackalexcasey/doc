COMMAND:
rpm2cpio kernel-2.6.9-1.667.src.rpm |cpio -idm

cp myinitrd mnt/initrd.img.gz
gunzip initrd.img.gz
cpio -i --make-directories < initrd.img

find ./ | cpio -H newc -o > initrd.img
gzip initrd.img
mv initrd.img.gz initrd.img


#if defined(CONFIG_MICREL_PHY_SETTING)  && defined (CONFIG_10_HALF)
#if (CONFIG_MICREL_PHY_SETTING && CONFIG_10_HALF)
/* Sysfs support */
#define OCP_SYSFS_ADDTL(type, format, name, field)   \
static ssize_t        \
show_##name##_##field(struct device *dev, struct device_attribute *attr, char *buf)   \
{         \
 struct ocp_device *odev = to_ocp_dev(dev);   \
 type *add = odev->def->additions;    \
         \
 return sprintf(buf, format, add->field);   \
}         \
static DEVICE_ATTR(name##_##field, S_IRUGO, show_##name##_##field, NULL);

void __init identify_ppc_sys_by_name(char *name)
{
 unsigned int i = 0;
 while (ppc_sys_specs[i].ppc_sys_name[0])
 {
  if (!strcmp(ppc_sys_specs[i].ppc_sys_name, name))
   break;
  i++;
 }
 cur_ppc_sys_spec = &ppc_sys_specs[i];
 return;
}

#if defined(CONFIG_MIPS)
#define TARGET_SYSCALL_NR_0  (__NR_N32_get_hist_buf - __NR_N32_Linux)
#elif defined(CONFIG_PPC)
#define TARGET_SYSCALL_NR_0  __NR_get_hist_buf
#elif defined(CONFIG_X86)
#define TARGET_SYSCALL_NR_0  __NR_sethostname
#else
ERROR_NO_ARCH _SPECIFIED
#endif

while [ 1 ] ; do echo tt;  sleep 20; done

while [ 1 ] ; do  cat /proc/kernel_patch/fake_oomd ;  sleep 20; done &

for each in *.tgz ; do tar xvfz $each ; done

#ifdef __KERNEL__

#else
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h> /* for atoi() */
#include <ctype.h> /* for isprint() */
#include <unistd.h>   /* for getpagesize() */
#include <fcntl.h>  /* for file open and close */

#define printk(args...) do{\
char buff[256];\
memset(buff,0,256);\
sprintf(buff,args);\
if (ppc_md.progress) ppc_md.progress(buff, 0x122);\
}while(0)

#define die( str ) fatal_error( str "\n" )

static void
fatal_error (const char *fmt_string)
{
  fprintf (stderr, fmt_string);
  exit (1);   /* The error exit code is 1! */
}

#define BUG()   do{\
        printf("\nBUG at %d\n",__LINE__);\
        exit(0);\
}while(0)
#define printk printf
#define panic printf

#define PRINTF_ERR(subsys,file,line,fmt, arg...) do {   \
 fprintf(stderr,fmt " %s, %s, %d \n", ## arg,subsys, file,line); \
} while (0)

#define SUBSYS_NAME "main"

void main(void)
{
 PRINTF_ERR(SUBSYS_NAME, __FILE__, __LINE__,"unable to open %s: %s\n", "sad", strerror(errno));

}

#define die(args...) do{ fprintf(stderr, args); exit(1) } while (0)

#ifdef PMEM_DEBUG
   /* send logs to /var/log/3pc.designlogs */
   #define pmem_log(args...) gen_design_log(__FUNCTION__, __FILE__, __LINE__, ## args)
   #define pmem_trace_log(args...) do { printf(args); fflush(stdout); } while (0)


   
   #define TAP_NAME(x) TAP_##x

#define FCN_TAP(tap_name,new_fcn_name) \
 void tap_name(void)\
 {\
  global_fcn_ptr =  (tap_fcn_t)new_fcn_name; \
  global_fcn_ptr();\
 }
        /*TAP 0*/
#define TARGET_FUNCTION_NAME  test1
#define SRC_FUNCTION_NAME  test2

FCN_TAP(TAP_NAME(0),NEW_FCN);

#define TAP_LINE_0 (tap_fcn_t)OLD_FCN,(tap_fcn_t)TAP_NAME(0),""

/*For every TAP fill in that structure*/
static struct kernel_fcn_patch kernel_fcn_patch_array[]= {
        {TAP_LINE_0},
 {NULL,NULL,""}
};

void main(void)
{
 TAP_();

 TAP_NAME(0)();

Driver initialisation...
__initcall(logmem_init);

init/main.c
 do_initcalls();

static int Debug =1;
#define PRINTF   if (Debug) printf


for(x=0;x<100;x++){
 if(!(x%2)){
  printf("\nevery 2");
 }
}


FORMATTAGE

 x=0;
 while(1){
  printf("\r\t\t\t %8ld",x);
  fflush(stdout);
  sleep(1);
  x++;
 }

char *GrabFile(char *name)
{
 unsigned long length;
 FILE *fh;
 char *ptr;

 fh= fopen(name,"r");
 if(fh==NULL){
   sprintf(err_string,"Cannot Open ADCP File %s",name);
   return NULL;
 }
/*Size the File*/
 fseek(fh,0L,SEEK_END);
  length = ftell(fh);
 fseek(fh,0L,SEEK_SET);

/*Allocate memory to Grab the ADCP File*/
 ptr = malloc(length);
 if(ptr == NULL){
   sprintf(err_string,"Memory Allocation Fail");
   fclose(fh);
   return NULL;
 }
 if(fread(ptr, sizeof(char), length , fh)!= length){
  sprintf(err_string,"Error Reading File %s",name);
   fclose(fh);
   free(ptr);
  return NULL;
 }
 PRINTF1("\nThe File %s is %ld bytes\n",name,length);
  fclose(fh);

 return ptr;
}

void main(void)
{
 char *ptr;
 ptr = GrabFile("/etc/shadow");
 printf("%s",ptr);
}



typedef struct cmd{
 char *cmd;
}cmdst;

int main(int argc, char **argv)
{
 int x;
 sectorst mysector;
 badblockst mybadblock;
 char cmd[500];
 cmdst cmdline[50];
  
 getSector(30,&mysector);
 getSector(44,&mysector);

/*Make the cmd line*/ 
 sprintf(cmd,"fdrawcmd read 4 2 1 10 2 0 0x1b 0xff length=512 need_seek track=2 >sect\n");
 cmdline[0].cmd = strtok(cmd," ");
 x=1;
 while((cmdline[x].cmd = strtok(NULL," "))!=NULL){
  x++;
 }
 submain(x,cmdline);
 
}


static void
get_list_blocks (char *filename)
{
  int i;
  FILE *listfile;
  unsigned long blockno;

  listfile = fopen (filename, "r");
  if (listfile == (FILE *) NULL)
    die ("Can't open file of bad blocks");

  while (!feof (listfile))
    {
      fscanf (listfile, "%ld\n", &blockno);
      for (i = 0; i < SECTORS_PER_BLOCK; i++) /* Mark all of the sectors in the block as bad */
 mark_sector_bad (blockno * SECTORS_PER_BLOCK + i);
      badblocks++;
    }
  fclose (listfile);

typedef void (*fcnptr)(void);

volatile fcnptr bin_entry;
unsigned long BinAddr;

void test(void)
{
 bin_entry = (fcnptr)BinAddr;
 (*bin_entry)();/*Jump on the Binary!*/
}


int main(int argc, char **argv)
{
 int verbose;
 int mode;

  if(!strcmp(argv[1],"ON")){
   verbose=1;
  }
  else{
   verbose=0;
  }

  if(!strcmp(argv[2],"Current")){
   mode=0;
  }



  void main(int argc, char *argv[])
{
 unsigned long g_virt_start_addr;


 printf("\n Entering download");
 g_virt_start_addr = init_shmem(SP_BASE_ADDRESS);
#ifdef SREC_DLD
 dld_main(g_virt_start_addr);
#else
 if(argc>1){
  printf("\n countindfgdf");
  sscanf(argv[1],"%lx",&myaddr);
 }
 printf("\nJump Addr = %lx",myaddr);
 
	
	
	
	
	
  	/* wait for the change of jiffies */
 ticks = jiffies;
 while (ticks == jiffies);

 /* read cpu counter */
 tmpStart = gethrtime();

 /* loop for another n jiffies */
 ticks += CALIBRATION_CYCLES + 1;
 while (ticks != jiffies);

 /* read counter again */
 tmpEnd = gethrtime();
 
 
 
 
 
        //EM...
        printk("\nOk lets see if udelay is doing is job");
        for(z=0;z<10;z++){/*10Sec*/
                for(x=0;x<1000;x++){/*1sec*/
                        for(y=0;y<10;y++){      /*1 Msec*/
                                udelay(100);    /*100usec*/
                        }
                }
                printk("\n%d Sec",z);
        }

        //EM...
        printk("\nOk lets see if Jiffies is OK");
        printk("\nHZ is set to be %d",HZ);

        for(z=0;z<10;z++){
                tmp=jiffies;
                do{/*1 Sec Loop...*/
                }while((jiffies - tmp) <HZ);
                printk("\n%d Sec",z);
        }

        //EM...
        printk("\nOk lets see if Jiffies/udelay is OK");
        for(z=0;z<10;z++){/*10Sec*/
                tmp=jiffies;
                for(x=0;x<1000;x++){/*1sec*/
                        for(y=0;y<10;y++){      /*1 Msec*/
                                udelay(100);    /*100usec*/
                        }
                }
                printk("\n%d Sec & jiffies=%d",z,(jiffies-tmp));
        }

	
	
/*CHAINED LIST*/
#ifdef PCI_DMA_1250_IOMMU
	if(!SB1250_MMUIO_ENTRY(ptr))
		return(0);
	/*First look if already mapped*/
	list_for_each(tmp, &sb1250_mmuio_list.head) {
		p=list_entry(tmp,struct sb1250_mmuio_list_head,head);
		if(SB1250_MMUIO_ENTRY(ptr) == p->addr_mmuio){/*Match*/
			p->usage++;
			/*TODO sanity check on the same value more that one!*/
			return(SB1250_PCI_ADDR(p->idx,ptr));/*Mapping to use*/
		}
	}
	/*Not mapped*/
	if(++DescriptorUsage >= SB1250_DESCRIPTOR_NUMBER)
		panic("out of IO descriptor");
#ifndef __KERNEL__
	p = (struct sb1250_mmuio_list_head*)malloc(sizeof(struct sb1250_mmuio_list_head));
#else
#endif
	if(p==NULL)
		panic("cannot allocate structure");

	list_add(&(p->head), &sb1250_mmuio_list.head);
	p->usage++;
	p->idx=DescriptorUsage;	/*TODO have a free pool of descriptor*/
	p->addr_mmuio=SB1250_MMUIO_ENTRY(ptr);
	SB1250_ADD_MMUIO(p->addr_mmuio,p->idx);
	return (SB1250_PCI_ADDR(p->idx,ptr));/*Mapping to use*/
/*

	for(x=0;x<10;x++){
		p = (struct sb1250_mmuio_list_head*)malloc(sizeof(struct sb1250_mmuio_list_head));
		list_add(&(p->head), &sb1250_mmuio_list.head);
		p->test=x;
	}

	list_for_each(tmp, &sb1250_mmuio_list.head) {
		p=list_entry(tmp,struct sb1250_mmuio_list_head,head);
		printf("\n%d",p->test);
	}
	return 0;*/

	
	
void dumpTLBtable(void * v_addr)
{
	unsigned long addr = (unsigned long) v_addr;
	pmd_t *pmd;
	pte_t *pte;
	pgd_t *pgd;
	int x;

	pgd = pgd_offset_k(addr);
	printk("\nAddr of the PDG =%lx",(unsigned long)pgd);
	for(x=0;x<10;x++){
		printk("\npgd[%d] = %lx",x,(unsigned long)pgd->pgd);
		pgd++;
	}

 pgd = pgd_offset_k(addr);

 printk("\nPMD_SHIFT =%lx",PMD_SHIFT);

 pmd = pgd->pgd;
 printk("\nAddr of the PMG =%lx",(unsigned long)pmd);
 for(x=0;x<10;x++){
  printk("\npmd[%d] = %lx",x,(unsigned long)pmd->pmd);
  pmd++;
 }

 pgd = pgd_offset_k(addr);
 pmd = pgd->pgd;
 pte = pmd->pmd;
 printk("\nAddr of the PTE =%lx",(unsigned long)pte);
 for(x=0;x<10;x++){
  printk("\npte[%d] = %lx",x,(unsigned long)pte->pte);
  pte++;
 }
} 



  if (ptrace (PTRACE_ATTACH, pid, 0, 0) != 0)
    {
      fprintf (stderr, "Cannot attach to process %d: %s (%d)\n", pid,
               errno < sys_nerr ? strerro(errno) : "unknown error",
               errno);
      fflush (stderr);
      _exit (0177);
    }

/*SUper macro*/
/* information message: e.g., configuration, major event */
#define jfs_info(fmt, arg...) do {   \
 if (jfsloglevel >= JFS_LOGLEVEL_INFO)  \
  printk(KERN_INFO fmt "\n", ## arg); \
} while (0)

/* debug message: ad hoc */
#define jfs_debug(fmt, arg...) do {   \
 if (jfsloglevel >= JFS_LOGLEVEL_DEBUG)  \
  printk(KERN_DEBUG fmt "\n", ## arg); \
} while (0)


#define __EXPORT_SYMBOL(sym, str)			\
const char __kstrtab_##sym[]				\
__attribute__((section(".kstrtab"))) = str;		\
const struct module_symbol __ksymtab_##sym 		\
__attribute__((section("__ksymtab"))) =			\
{ (unsigned long)&sym, __kstrtab_##sym }
#define EXPORT_SYMBOL(var)  __EXPORT_SYMBOL(var, __MODULE_STRING(__VERSIONED_SYMBOL(var)))

#define __MODULE_STRING_1(x)	#x
#define __MODULE_STRING(x)	__MODULE_STRING_1(x)



/*HOW to read ksym tab*/
extern const char __kstrtab_is_bad_inode[];
extern const char __kstrtab_event[];
extern const char __kstrtab_brw_page[];



extern const char __kstrtab_Test123[];

static int init(void * unused)
{
	unsigned long *ptr;
	lock_kernel();
	do_basic_setup();

//	test1_printk("Helloinit");
	printk("\n%s",__kstrtab_is_bad_inode);
	printk("\n%s",__kstrtab_event);
	printk("\n%s",__kstrtab_brw_page);
	printk("\n%s",__kstrtab_Test123);
	

A UNIQUE variable name is created in the Specific section

objdump -h vmlinux

Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00224368  80100000  80100000  00001000  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .fixup        000024c8  80324368  80324368  00225368  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .kstrtab      00005788  80326830  80326830  00227830  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 __ex_table    00004b70  8032bfc0  8032bfc0  0022cfc0  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 __dbe_table   00000000  80330b30  80330b30  00231b30  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 __ksymtab     000048f0  80330b30  80330b30  00231b30  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA

kstrtab
Use the VMA addr caus e it's a loaded time...
0x80326830	->326830 {memory view}	->3303472
5788 ->			22408

dd if=/dev/mem of=test bs=1 skip=3303472 count=22408

  3 .kstrtab      00008490  9016ba78  9016ba78  0017ba78  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
9016ba78	->16ba78		->1489528
8490		->33936
dd if=/dev/mem of=test bs=1 skip=1489528 count=33936

cat test
~ # cat test
disable_irq_nosyncdisable_irqenable_irqprobe_irq_maskmips_io_port_baseisa_slot_o
ffsetrtc_lockto_tmrtc_set_timertc_get_timemips_machtypememcmpmemsetmemcpymemmove
strcatstrchrstrlenstrpbrkstrncatstrnlenstrrchrstrtokstrstrstrcpystrncpystrcmpstr
ncmpmemscan_clear_pagekernel_thread__copy_user__bzero__strncpy_from_user_nocheck
_asm__strncpy_from_user_asm__strlen_user_nocheck_asm__strlen_user_asm__strnlen_u


cat /proc/ksyms
ffffffff80104d10 disable_irq_nosync
ffffffff80103d48 disable_irq
ffffffff80103e38 enable_irq
ffffffff80104608 probe_irq_mask
ffffffff802fd7a0 mips_io_port_base
ffffffff8069a340 isa_slot_offset
ffffffff8069a8d0 rtc_lock
ffffffff8010c9b8 to_tm
ffffffff803f7b18 rtc_set_time
ffffffff803f7b10 rtc_get_time
ffffffff8069a180 mips_machtype
ffffffff802f7dd0 memcmp
ffffffff802f7360 memset
ffffffff802f74a0 memcpy
ffffffff802f7780 memmove
ffffffff802f79b8 strcat
ffffffff802f7ac8 strchr
ffffffff802f7b70 strlen
ffffffff802f7c60 strpbrk
ffffffff802f79f0 strncat
ffffffff802f7b98 strnlen
ffffffff802f7b08 strrchr

fgrep -b -a kernel /dev/mem
dd if=/dev/mem bs=1M | od -X | more
 dd if=/dev/mem bs=1M | od -t x4 | grep c0de4ced
 dd if=/dev/mem bs=1M count=7 | od -t x4 | grep 534d4901

#define SECOND_SW_DELAY 250000
#define TRAP_CHAR 0x10 /*Trap the CTRL-P*/
int IsKeyPress(void)
{
  struct termios new_settings;
  struct termios saved_settings;
	int a,x;

  tcgetattr(0,&saved_settings);
  new_settings = saved_settings;
  /* Disable canonical mode, and set buffer size to 1 byte */
  new_settings.c_lflag &= (~ICANON);
  new_settings.c_lflag &= (~ECHO);
  //new_settings.c_lflag &= (~ISIG);
  new_settings.c_cc[VTIME] = 0;
  new_settings.c_cc[VMIN] = 0; /*return if no char available*/

  tcsetattr(0,TCSANOW,&new_settings);

//  PRINTF("Step 4:");
	x=SECOND_SW_DELAY*2;	/*Approx :-)*/
	do{
		a = getchar();
		x--;
		if(x %(SECOND_SW_DELAY/5) ==0){			
//				PRINTF(".");
		}
#if 0
		if(a!= -1){       /*Display all Character*/
			PRINTF("%x ",a);
		}
#endif
	}while((a != TRAP_CHAR) && (x>0));

	if(a==TRAP_CHAR){
	  tcsetattr(0,TCSANOW,&saved_settings);
		return 1;
	}
 	tcsetattr(0,TCSANOW,&saved_settings);
	return 0;
}




/****************************************************************************/

void set_keyboard(){
  setvbuf(stdout,NULL,_IONBF,0);

  tcgetattr(0,&saved_settings);
  new_settings = saved_settings;

  /* Disable canonical mode, and set buffer size to 1 byte */
  new_settings.c_lflag &= (~ICANON);
  new_settings.c_lflag &= (~ECHO);
  new_settings.c_lflag &= (~ISIG);      // don't answer those in webOS...
  new_settings.c_cc[VTIME] = 0;
  new_settings.c_cc[VMIN] = 0; /*return if no char available*/

  tcsetattr(0,TCSANOW,&new_settings);
}

void close_keyboard(){
  tcsetattr(0,TCSANOW,&saved_settings);
  setvbuf(stdout,NULL,_IOLBF,0);
}


	printk("\nDebug Stuff");

	printk("\nkmalloc start = %lx",PAGE_OFFSET);
	printk("\nPAGE_SIZE= %lx",PAGE_SIZE);

	printk("\nVMALLOC_START = %lx",VMALLOC_START );
	printk("\nVMALLOC_END = %lx",VMALLOC_END );

	printk("\narea a second-level page table can map");
	printk("\nPMD_SHIFT = %lx",PMD_SHIFT );
	printk("\nPMD_SIZE = %lx",PMD_SIZE );

	printk("\nwhat a third-level page table entry can map");
	printk("\nPGDIR_SIZE = %lx",PGDIR_SIZE );
	printk("\nPGDIR_SHIFT = %lx",PGDIR_SHIFT );

	printk("\nUser space process size");
	printk("\nTASK_SIZE = %lx",TASK_SIZE );

	printk("\nwhere the kernel will search for a free chunk of vm space during mmap's");
	printk("\nTASK_UNMAPPED_BASE = %lx", TASK_UNMAPPED_BASE);

	printk("\n If get_fs() == USER_DS, checking is performed, with get_fs() == KERNEL_DS, checking is bypassed.");
	printk("\nUSER_DS =%lx",USER_DS);
	printk("\nKERNEL_DS =%lx",KERNEL_DS);

	printk("\nUSER_PTRS_PER_PGD =%lx",USER_PTRS_PER_PGD);


	
	
	+char *ptr =NULL;
+#include <linux/time.h>
+#include <linux/timex.h>
+
 static int init(void * unused)
 {
+       int z,x;
+       unsigned long tmp;
+
+       printk("While 1 the Kernel\n");
+#if 1
+       x=0;
+       while(x<5){
+
+               for(z=0;z<10;z++){
+                       tmp=jiffies;
+                       do{/*1 Sec Loop...*/
+                       }while((jiffies - tmp) <HZ);
+               }
+               printk("%d Sec.........................................\n",x*10);
+               x++;
+       }
+#endif
+       printk("While 1 the Kernel DONE\n");
+
+
+       /*Oupssssss*/
+       *ptr =1;
+


###################PROC



#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>

//extern unsigned long highmem_start_page;
#define SANITY_HIGHMEM(ptr,page) do{\
 if(!ptr)\
  panic("Cannot map HighMem page");\
 if (page < highmem_start_page)\
  printk("\nPage is not in HIGHMEM!!!");\
}while(0)

#define COPY_FROM_USER(error,dest,src,size) error = copy_from_user(dest,src,size) ? -EFAULT : 0
#define COPY_TO_USER(error,dest,src,size) error = copy_to_user(dest,src,size) ? -EFAULT : 0
#define PRINT_PROC(fmt,args...)       \
 do {            \
  *len += sprintf( buffer+*len, fmt, ##args ); \
  if (*begin + *len > offset + size)    \
   return( 0 );        \
  if (*begin + *len < offset) {     \
   *begin += *len;        \
   *len = 0;         \
  }            \
 } while(0)

static struct proc_dir_entry *em_debug_proc;
static spinlock_t em_debug_lock;

/*
 *
 *
 */
static int em_debug_proc_info( unsigned char *buf, char *buffer, int *len,
            off_t *begin, off_t offset, int size, int mode)
{
	PRINT_PROC("\n/*****pci_dma_iommu_Zone*****/");
}
/*
 *
 *
 */
static int em_debug_read_proc( char *buffer, char **start, off_t offset,
		int size, int *eof, void *data )
{
	int len = 0;
	off_t begin = 0;

	spin_lock(&em_debug_lock);
	*eof = em_debug_proc_info( NULL , buffer, &len, &begin, offset, size,0 );
	spin_unlock(&em_debug_lock);

	if (offset >= begin + len)
		return( 0 );
	*start = buffer + (offset - begin);  // CORRECTED !!!!
	return( size < begin + len - offset ? size : begin + len - offset );
}


typedef struct{
	char element[20];
}token_st;
token_st token_array[5];

int em_debug_user_command(char *user_command)
{
	char *ptr;
	token_st *token_ptr =token_array;
	unsigned long currpfn,vaddr,paddr;
	struct page* page;
	char *tmpptr=NULL;
	int x,value;

	printk("%s\n",user_command);
	ptr = (char*)strtok(user_command," ");

	while(ptr!=NULL){
		strcpy((char*)(token_ptr->element),ptr);
		ptr = (char*)strtok(NULL," ");
		token_ptr++;
	}
	if(!strcmp(token_array[0].element, "rdpfn")){
		sscanf(token_array[1].element,"%lx",&currpfn);
		printk("rdpfn -> %lx\n",currpfn);
	}

	if(!strcmp(token_array[0].element, "rdvaddr")){/*???*/
		sscanf(token_array[1].element,"%lx",&vaddr);
		printk("rdvaddr -> %lx\n",vaddr);
		page = virt_to_page(vaddr);
		if (!VALID_PAGE(page))
			BUG();
		tmpptr =kmap(page);
		printk("Highmem ptr =%lx",(unsigned long)tmpptr);
		SANITY_HIGHMEM(tmpptr,page);
		for(x=0;x<PAGE_SIZE;x++){
			printk("%x",tmpptr[x]);
			if(!(x%16)){
				printk("\n");
			}
		}
		kunmap(page);
	}
	if(!strcmp(token_array[0].element, "rdpaddr")){
		sscanf(token_array[1].element,"%lx",&paddr);
		printk("rdpaddr -> %lx\n",paddr);
		page =  (struct page*)(mem_map+(paddr>>PAGE_SHIFT));
		if (!VALID_PAGE(page))
			BUG();
		tmpptr =kmap(page);
		printk("Highmem ptr =%lx",(unsigned long)tmpptr);
		SANITY_HIGHMEM(tmpptr,page);
		printk("\n");
		for(x=0;x<PAGE_SIZE;x++){
			printk("%x",tmpptr[x]);
			if(!(x%16)){
				printk("\n");
			}
		}
		kunmap(page);
	}

	if(!strcmp(token_array[0].element, "wrpaddr")){
		sscanf(token_array[1].element,"%lx",&paddr);
		sscanf(token_array[2].element,"%lx",&value);
		printk("wrpaddr -> %lx=%x\n",paddr,value);
		page =  (struct page*)(mem_map+(paddr>>PAGE_SHIFT));
		if (!VALID_PAGE(page))
			BUG();
		tmpptr =kmap(page);
		printk("Highmem ptr =%lx",(unsigned long)tmpptr);
		SANITY_HIGHMEM(tmpptr,page);
		memset(tmpptr,value,PAGE_SIZE);
		kunmap(page);
	}
}

/*
 *
 *
 */
static int em_debug_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char command_buffer[80];
	int rc, length;

	/* write only allowed for root users */
	if (current->euid != 0) return -EACCES;

	if (count > sizeof(command_buffer)-1) return -EINVAL;

  	COPY_FROM_USER(rc, command_buffer, buffer, count);
	if (rc) { return rc; }

  	command_buffer[count] = '\0';
  	length = strlen(command_buffer);
  	if (command_buffer[length-1] == '\n')
		command_buffer[--length] = '\0';

	spin_lock(&em_debug_lock);
	rc=em_debug_user_command(command_buffer);
	spin_unlock(&em_debug_lock);
	return (rc ? count : -EBUSY);
}

void em_debug_procinit(void)
{
	spin_lock_init(&em_debug_lock);
	if ((em_debug_proc = create_proc_entry( "em_debug", S_IFREG|S_IRUGO|S_IWUSR, 0 ))) {
		em_debug_proc->read_proc = em_debug_read_proc;
		em_debug_proc->write_proc = em_debug_write_proc;
	}
}




strtok EX
char teststring[100]="rdpfn 10 32";

void main(void)
{
 char *ptr;

 ptr = teststring;

 ptr = (char*)strtok(teststring," ");
 while(ptr!=NULL){
  printf("\nToken %s",ptr);
  ptr = (char*)strtok(NULL," ");
 }
 exit();
}



 /* Make it uncached.
 */
 pte = va_to_pte(mem_addr);
 pte_val(*pte) |= _PAGE_NO_CACHE;
 flush_tlb_page(init_mm.mmap, mem_addr);

/*
 * ----------------------------------------------------------------------------
 * Allocate the specified number of pages and mark them uncached.
 * ASSUME: kmalloc() supplies page-aligned memory
 *         mapped to contiguous physical pages
 */
u32
uncachedPages(u32 pages)
{
 pte_t *pte;
 u32 addr;
 u32 firstPage;

 firstPage = addr = (u32) kmalloc((pages * PAGE_SIZE), GFP_KERNEL);
 if (!addr || (addr & ~PAGE_MASK)) {
  panic("uncachedPages: can't get page-aligned memory.\n");
	}
	while (pages--) {
		pte = va_to_pte(addr);
		pte_val(*pte) |= (_PAGE_NO_CACHE | _PAGE_GUARDED);
		flush_tlb_page(init_mm.mmap, addr);
		invalidate_dcache_range(addr, addr + PAGE_SIZE);
		addr += PAGE_SIZE;
	}
	mb();
	return (firstPage);
}


      /* open dev/bigfoot, and set the file descriptor in our global variable */
      pmem_ctl_blk.fd = open(PERSISTENT_MEMORY_DEVICE, O_RDWR|O_CREAT|O_SYNC,S_IRUSR|S_IWUSR);
      if (pmem_ctl_blk.fd < 0)
      {
         pmem_log("pmem ERROR: could not open %s!!!\n", PERSISTENT_MEMORY_DEVICE);
         return -1;
      }
   

      /* now map the whole of our persistent memory area as "unsable" (we'll then map chunks of
      it as usable, while leaving unusable buffer pages inbetween the usable chunks) */

      /* arg 1: 0 --> logical start address decided by mmap
         arg 2: size of memory to mmap (in bytes)
         arg 3: PROT_NONE --> DO NOT allow read or write
         arg 4: MAP_SHARED --> mapping shared with all other processes that map to /dev/bigfoot
                              (ie. storing to the region is equivalent to writing to the persistent
                                   memory, but the physical memory may not be actually updated unless
                                   msync(2) or munmap(2) are called)
                | MAP_ANONYMOUS --> mmap does not use memory from any real file or device
         arg 5: 0 --> no corresponding file descriptor
         arg 6: 0 --> no offset into a file descriptor 
      */
      tmp_ptr = mmap(0, PMEM_LOGICAL_SIZE, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
      if ((int)tmp_ptr < 0)
      {
         printf("Error: pmem area init ERROR= %d\n", errno);
         return -1;
      }
      else
      {
         /* save address to beginning of pmem area, in our global var */
         pmem_ctl_blk.pmem_address = (unsigned int)tmp_ptr;
      }
      
      /* the logical address of the control block will be 1 page into the unreadable area we've mmap'd */
      logical_control_block_address = pmem_ctl_blk.pmem_address + PMEM_PAGE;

      /* unmap the area first */
      munmap_result = munmap((void*)logical_control_block_address, PMEM_PAGE);
      
      if (munmap_result < 0)
      { pmem_log("munmap error: %d\n", errno); return -1; }

      /* now mmap a valid chunk of data from dev/bigfoot (for the control data), 
        1 page into the logical unusable memory block we just mmap'd anonymously*/
     
      /* arg 1: unusable_mem + PMEM_PAGE --> 1 buffer page down from the start of our pmem logical memory space
         arg 2: size of memory to mmap (in bytes)
         arg 3: allow read and write
         arg 4: MAP_SHARED --> mapping shared with all other processes that map to /dev/bigfoot
                              (ie. storing to the region is equivalent to writing to the persistent
                                   memory, but the physical memory may not be actually updated unless
                                   msync(2) or munmap(2) are called)
                | MAP_FIXED -->  the address returned by mmap MUST be arg 1  (we want the memory to be contiguous)
         arg 5: file descriptor of the persistent memory
         arg 6: offset into the file descriptor that we want to start mapping at
      */
      (pmem_ctl_blk.control_data) = (PMemControlStruct *)mmap((void*)logical_control_block_address,
                                                           PMEM_PAGE,
                                                           PROT_READ|PROT_WRITE,
                                                           MAP_SHARED | MAP_FIXED,
                                                           pmem_ctl_blk.fd,
                                                           PERSISTENT_MEMORY_OFFSET);

      if ((int)pmem_ctl_blk.control_data < 0 )
         {pmem_log("control data ERROR= %d, error: %m\n", errno);return -1;}




static int bigfoot_mmap(struct file * file, struct vm_area_struct * vma)
{
  unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

  if (offset & ~PAGE_MASK)
      return -ENXIO;

  /*
   * Accessing memory through a file pointer that was marked O_SYNC
   * will be done non-cached.
   */
  if (file->f_flags & O_SYNC)
      pgprot_val(vma->vm_page_prot)
          = logmem_pgprot_noncached(pgprot_val(vma->vm_page_prot));

  /* Does the mapping start beyond the end of the device? */
  if ((bigfoot_end - bigfoot_start) <= offset)
     return -EINVAL;

  /* Does the mapping end beyond the end of the device? */
  if ((bigfoot_end - (bigfoot_start + offset)) < (vma->vm_end - vma->vm_start))
     return -EINVAL;

  if (remap_page_range(vma->vm_start, bigfoot_start + offset, vma->vm_end-vma->vm_start,
               vma->vm_page_prot))
      return -EAGAIN;
  return 0;
}

struct file_operations bigfoot_fops = {
 llseek:  logmem_lseek,
 read:  bigfoot_read,
 write:  bigfoot_write,
 mmap:  bigfoot_mmap,
};



/* Check that a process has enough memory to allocate a
 * new virtual mapping.
 */
int vm_enough_memory(long pages)
{






void main(int argc, char*argv[])
{
 int rate,time,try;
 pid_t pid;

 if(argc <4){
  printf("Wrong arg Number\n");
  exit(0);
 }

	sscanf(argv[1],"%lx",&rate);
	sscanf(argv[2],"%lx",&time);
	sscanf(argv[3],"%lx",&try);


int main(int argc, char**argv)
{
	int rate,time,maxmem;
	pid_t pid;
	unsigned char *ptr=NULL;
	unsigned long max_malloc_size=MAX_VM_MEMORY;
	int c;

	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;

	if (argc && *argv)
		program_name = *argv;

	while ((c = getopt (argc, argv, "bfhixVo:")) != EOF) {
		switch (c) {
		case 'V':
			/* Print version number and exit */
			fprintf(stderr, "\tUsing %s %s\n",MLEAK_VERSION,MLEAK_DATE);
			exit(0);
			break;
		case 'o':
			if (optarg[0] == 'b')
				use_superblock = atoi(optarg+1);
			else if (optarg[0] == 'B')
				use_blocksize = atoi(optarg+1);
			else
				usage();
		default:
			usage();
		}
	}
	if (optind > argc - 1)
		usage();

	{ /* narrow the scope */
		int index = 0;
		for( index = optind; index < argc; index++ )
			printf( "Non-option argument \"%s\"\n", argv[index] );
	}



	 
#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>

#include <linux/module.h>
#include <linux/config.h>
#include <linux/version.h>   /* Specifically, a module */
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>

#include "cheetah_irq.h"

static int Debug =0;

#define PRINTK   if (Debug) printk

/*****************************************POSIX**********************************/
/*****************************************POSIX**********************************/
/*****************************************POSIX**********************************/
/*****************************************POSIX**********************************/

static int Device_Open = 0;

static ssize_t device_write(struct file *file, const char *buf, size_t count,
                            loff_t *offset)
{
 int err =-EINVAL;
 unsigned char temp;
 
 get_user(temp, buf++);
 err = count;
  return err;
}

static ssize_t device_read (struct file *file, char *buf, size_t count,
                            loff_t *offset)
{
 int err =-EINVAL;
 unsigned char temp;

 temp ='a';
 put_user(temp, buf++);
  err = count;
  return err;
}


static int device_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
                  unsigned long arg)
{
 int err = -EINVAL;

 switch (cmd) {
  case 0:
   err = SUCCESS;
   break;
  
  default:
   printk("Error in Cmd Number \n");
			err =  -EINVAL;
			break;
	}
	return err;	
}



static int device_open(struct inode *inode,struct file *file)
{
  PRINTK("Device: %d.%d\n",inode->i_rdev >> 8, inode->i_rdev & 0xFF);
  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  MOD_INC_USE_COUNT;
		
  return SUCCESS;
}

static int device_close(struct inode *inode, struct file *file)
{
  PRINTK("Device_release: %d.%d\n",inode->i_rdev >> 8, inode->i_rdev & 0xFF);
  Device_Open --;
  MOD_DEC_USE_COUNT;
  return 0;
}

static struct file_operations Fops = {
	read:			device_read,
	write:		device_write,	
	ioctl:		device_ioctl,
	open:			device_open,
	release:	device_close,
};

int init_module(void)
{
  int err;

  err = register_chrdev(MY_MAJOR,DEVICE_NAME,&Fops);
  if (err) {
    PRINTK ("Device failed with %d\n",err);
    unregister_chrdev(MY_MAJOR,DEVICE_NAME);
    return err;
  }
  printk ("Device %s inserted\n",DEVICE_NAME);
  return SUCCESS;
}

int cleanup_module(void)
{

  unregister_chrdev(MY_MAJOR,DEVICE_NAME);
	printk ("Device %s removed\n",DEVICE_NAME);
  return SUCCESS;
}

EXPORT_NO_SYMBOLS;












/**********************FIFO******************/


static unsigned char Rx_Buffer[RX_BUFSIZE];
static int Rx_BufferHead =0; //Always Point to an Empty
static int Rx_BufferTail =0; //Always Point to a Full
static unsigned char Tx_Buffer[TX_BUFSIZE];
static int Tx_BufferHead =0; //Always Point to an Empty
static int Tx_BufferTail =0; //Always Point to a Full

static int IsRx_BufferFull (void);
static int IsRx_BufferDataAvailable (void);
static int IsTx_BufferFull (void);
static int IsTx_BufferDataAvailable (void);
int Tx_BufferRead (unsigned char *value);
int Tx_BufferWrite (unsigned char value);
void Tx_BufferFlush (void);
int Rx_BufferRead (unsigned char *value);
int Rx_BufferWrite (unsigned char value);
void Rx_BufferFlush (void);

/* 
0 -> Read Succesfull.
1 -> Read Fail.
*/
int Tx_BufferRead (unsigned char *value)
{
 if (!IsTx_BufferDataAvailable()){
  return 1;
 }
 *value = Tx_Buffer[Tx_BufferTail];
 if (Tx_BufferTail == (TX_BUFSIZE - 1)){
		Tx_BufferTail = 0;
	}
	else{
		Tx_BufferTail++;				
	}
	return 0;
}


/*
0 -> Read Succesfull.
1 -> Read Fail.
*/
int Rx_BufferRead (unsigned char *value)
{
 if (!IsRx_BufferDataAvailable()){
  return 1;
 }
 *value = Rx_Buffer[Rx_BufferTail];
 if (Rx_BufferTail == (RX_BUFSIZE - 1)){
  Rx_BufferTail = 0;
 }
 else{
  Rx_BufferTail++;
 }
 return 0;
}

/*
0 -> Write SuccesFull.
1 -> Write Fail.
*/
int Rx_BufferWrite (unsigned char value)
{
 if(IsRx_BufferFull()){
  return 1;
 }
 Rx_Buffer[Rx_BufferHead] = value;

 if (Rx_BufferHead == (RX_BUFSIZE - 1)){
  Rx_BufferHead = 0;
 }
 else{
  Rx_BufferHead++;
 }
 return 0;
}

/*
0 -> Write SuccesFull.
1 -> Write Fail.
*/
int Tx_BufferWrite (unsigned char value)
{
	if(IsTx_BufferFull()){
		return 1;
	}
	Tx_Buffer[Tx_BufferHead] = value;

	if (Tx_BufferHead == (TX_BUFSIZE - 1)){
		Tx_BufferHead = 0;
	}
	else{
		Tx_BufferHead++;
	}
	return 0;
}


void Rx_BufferFlush (void)
{
	Rx_BufferHead =0;
	Rx_BufferTail =0;
}

void Tx_BufferFlush (void)
{
	Tx_BufferHead =0;
	Tx_BufferTail =0;
}



/*
0 ->	Nothing Available.
1 ->	Data Available.
*/
static int IsRx_BufferDataAvailable (void)
{
	return (Rx_BufferHead != Rx_BufferTail);
}

/*
1 -> Buffer is ALMOST full.
0 -> Buffer is NOT full.
ALMOST when Remain Only _BUFMIN Free
*/
static int IsRx_BufferFull (void)
{
	int temp;

	if(Rx_BufferHead >= Rx_BufferTail){
		temp = (Rx_BufferHead - Rx_BufferTail);
		temp = (RX_BUFSIZE)-temp;
	}
	else{
		temp = (Rx_BufferTail - Rx_BufferHead);
	}
 return (temp <= RX_BUFMIN);
}

/*
0 -> Nothing Available.
1 -> Data Available.
*/
static int IsTx_BufferDataAvailable (void)
{
 return (Tx_BufferHead != Tx_BufferTail);
}

/*
1 -> Buffer is ALMOST full.
0 -> Buffer is NOT full.
ALMOST when Remain Only _BUFMIN Free
*/
static int IsTx_BufferFull (void)
{
 int temp;

 if(Tx_BufferHead >= Tx_BufferTail){
  temp = (Tx_BufferHead - Tx_BufferTail);
  temp = (TX_BUFSIZE)-temp;
 }
 else{
  temp = (Tx_BufferTail - Tx_BufferHead);
 }
 return (temp <= TX_BUFMIN);
}




/* FILE NAME BASE ON THE FS ENTRY...
static ssize_t pid_maps_read(struct file * file, char * buf,
         size_t count, loff_t *ppos)
{
 struct inode * inode = file->f_dentry->d_inode;
 struct task_struct *task = inode->u.proc_i.task;
 ssize_t res;

 res = proc_pid_read_maps(task, file, buf, count, ppos);
 return res;
}

static struct file_operations proc_maps_operations = {
	read:		pid_maps_read,
};
*/


typedef unsigned long (*fcnptr)(struct mm_struct *,struct file *,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long);
 /* System.map
  * 9002cde4 T do_mmap_pgoff_mm
  */

 #define FCNPTR_DO_MMAP_PGOFF  0x9002cde4

       volatile fcnptr bin_entry;

       bin_entry = (fcnptr)FCNPTR_DO_MMAP_PGOFF;
       return ((*bin_entry)(current->mm,file,addr,len,prot,flags,pgoff));

       //return do_mmap_pgoff(file, addr, len, prot, flags, pgoff);

       
       
curr_log=curr_log ? 0:1;/*Toggle to point to Active / Inactive log*/


















#define DYN_LOG_LHB()	do{\
	hist_buf_info.hist_buf_ptr = ( hist_buf_struct *) phys_to_virt(iopa((unsigned long)DYN_LOG_ACTIVE.lhb_buf));\
	hist_buf_info.hist_buf_start = ( hist_buf_struct *)phys_to_virt(iopa((unsigned long)DYN_LOG_ACTIVE.lhb_buf));\
	hist_buf_info.hist_buf_end = ( hist_buf_struct *)phys_to_virt(iopa(((unsigned long)DYN_LOG_ACTIVE.lhb_buf) + PAGE_SIZE));\
	hist_buf_info.hist_buf_record_flag = 1;\
}while(0)

/*	ptr=ioremap(kcore_start, PANIC_CAPTURE_BUF_SIZE);
	if(!ptr){
		printk("Kernel core-dump, cannot ioremap kcore_start at %lx\n",kcore_start);
		return 1;
	}

	kernel_kcore =(struct kcore_section *) ptr;
	*/




#if 1
	int x;
	{
		printk("hist_buf_info.hist_buf_ptr = %lx\n",(unsigned long)hist_buf_info.hist_buf_ptr);
		printk("hist_buf_info.hist_buf_start = %lx\n",(unsigned long)hist_buf_info.hist_buf_start);
		printk("hist_buf_info.hist_buf_end = %lx\n",(unsigned long)hist_buf_info.hist_buf_end);

		printk("DYN_LOG_INACTIVE.lhb_buf = %lx\n",(unsigned long)DYN_LOG_INACTIVE.lhb_buf);
		printk("DYN_LOG_ACTIVE.lhb_buf = %lx\n",(unsigned long)DYN_LOG_ACTIVE.lhb_buf);
	printk("iopa((unsigned long)DYN_LOG_ACTIVE.lhb_buf) %lx\n",iopa((unsigned long)DYN_LOG_ACTIVE.lhb_buf));
	printk("iopa((unsigned long)DYN_LOG_INACTIVE.lhb_buf) %lx\n",iopa((unsigned long)DYN_LOG_INACTIVE.lhb_buf));


	}

#if 1
	{
               hist_buf_struct *hb_pointer;
               int hb_i;

               printk(KERN_EMERG "Dumping linux exception history:\n"
                       "%s, %s, %s, %s, %s\n",
                       "nip", "link", "trap", "last syscall", "low tbr");
               hb_pointer = hist_buf_info.hist_buf_start;

               while(hb_pointer < hist_buf_info.hist_buf_end)
                       {
                       printk( "%08X,%08X,%04X,%04X,%08X;\n",
                               (unsigned int) hb_pointer->hbnip,
                               (unsigned int) hb_pointer->hblink,
                               hb_pointer->hbtrap, hb_pointer->hbsc,
                               (unsigned int) hb_pointer->hbtl);

                               hb_pointer += 1;
                       }
 	}
#endif
	for(x=0;x<20;x++){
		printk("%x ",DYN_LOG_INACTIVE.lhb_buf[x]);
	}
	printk("\n");
	for(x=0;x<20;x++){
		printk("%x ",DYN_LOG_ACTIVE.lhb_buf[x]);
	}
 printk("\n");
#endif

 pte_t * tmp_pte=NULL;
 printk("Setting Cache attribute\n");
 tmp_pte =va_to_pte((unsigned long)DYN_LOG_IDX(0).lhb_buf);
 if(tmp_pte){
  PRINT_PAGE_BIT(*tmp_pte);
  ptep_mkwritethrough(tmp_pte);
  PRINT_PAGE_BIT(*tmp_pte);
 }

 tmp_pte =va_to_pte((unsigned long)DYN_LOG_IDX(1).lhb_buf);
 if(tmp_pte){
  PRINT_PAGE_BIT(*tmp_pte);
  ptep_mkwritethrough(tmp_pte);
  PRINT_PAGE_BIT(*tmp_pte);
 }
static inline int pte_checkcache(pte_t pte)  { return pte_val(pte) & _PAGE_WRITETHRU; }
static inline void pte_writethrough(pte_t pte)       { pte_val(pte) |= _PAGE_WRITETHRU; }

static inline void ptep_mkwritethrough(pte_t *ptep)
{
 pte_update(ptep, 0, _PAGE_WRITETHRU);
}

#define PRINT_PAGE_BIT(pte) do{\
 printk("Read ?%d\n",pte_read(pte) ?1:0);\
 printk("Write ?%d\n",pte_write(pte) ?1:0);\
 printk("Dirty ?%d\n",pte_dirty(pte) ?1:0);\
 printk("Young ?%d\n",pte_young(pte) ?1:0);\
 printk("Exec ?%d\n",pte_exec(pte) ?1:0);\
 printk("cache ?%d\n",pte_checkcache(pte) ?1:0);\
}while(0)
/* printk("Uncache ?%d\n",pte_uncache(pte) ?1:0);\
 printk("Cache ?%d\n",pte_cache(pte) ?1:0);\
*/
#if 0
#define _PAGE_WRITETHRU 0x040 /* W: cache write-through */
#define _PAGE_DIRTY 0x080 /* C: page changed */
#define _PAGE_ACCESSED 0
#endif
#include <asm/pgtable.h>


#if 0
	{
		printk("hist_buf_info.hist_buf_ptr = %lx\n",(unsigned long)hist_buf_info.hist_buf_ptr);
		printk("hist_buf_info.hist_buf_start = %lx\n",(unsigned long)hist_buf_info.hist_buf_start);
		printk("hist_buf_info.hist_buf_end = %lx\n",(unsigned long)hist_buf_info.hist_buf_end);

		printk("DYN_LOG_INACTIVE.lhb_buf = %lx\n",(unsigned long)DYN_LOG_INACTIVE.lhb_buf);
		printk("DYN_LOG_ACTIVE.lhb_buf = %lx\n",(unsigned long)DYN_LOG_ACTIVE.lhb_buf);
	}
#endif
#if 0
	{
	pte_t * tmp_pte=NULL;

	printk("Setting Cache attribute\n");
	tmp_pte =va_to_pte((unsigned long)DYN_LOG_IDX(0).lhb_buf);
	if(tmp_pte){
		PRINT_PAGE_BIT(*tmp_pte);
		ptep_mkwritethrough(tmp_pte);
		PRINT_PAGE_BIT(*tmp_pte);
	}

	tmp_pte =va_to_pte((unsigned long)DYN_LOG_IDX(1).lhb_buf);
	if(tmp_pte){
		PRINT_PAGE_BIT(*tmp_pte);
		ptep_mkwritethrough(tmp_pte);
		PRINT_PAGE_BIT(*tmp_pte);
	}

	}

#if 1
	{
               hist_buf_struct *hb_pointer;


               printk("Dumping linux exception history:\n"
                       "%s, %s, %s, %s, %s\n",
                       "nip", "link", "trap", "last syscall", "low tbr");
		if(curr_log){
               		hb_pointer = ( hist_buf_struct *)DYN_LOG_INACTIVE.lhb_buf;


               		while(hb_pointer < ((( hist_buf_struct *)(DYN_LOG_INACTIVE.lhb_buf)) + HIST_BUF_N))
                       {
                       printk( "%08X,%08X,%04X,%04X,%08X;\n",
                               (unsigned int) hb_pointer->hbnip,
                               (unsigned int) hb_pointer->hblink,
                               hb_pointer->hbtrap, hb_pointer->hbsc,
                               (unsigned int) hb_pointer->hbtl);

                               hb_pointer += 1;
                       }
		}
		else{
		hb_pointer = ( hist_buf_struct *)DYN_LOG_ACTIVE.lhb_buf;
  		while(hb_pointer < ((( hist_buf_struct *)(DYN_LOG_ACTIVE.lhb_buf)) + HIST_BUF_N))
                       {
                       printk( "%08X,%08X,%04X,%04X,%08X;\n",
                               (unsigned int) hb_pointer->hbnip,
                               (unsigned int) hb_pointer->hblink,
                               hb_pointer->hbtrap, hb_pointer->hbsc,
                               (unsigned int) hb_pointer->hbtl);

                               hb_pointer += 1;
                       }
		}
 	}
#endif
#endif

volatile int a;
void sw_delay(unsigned long value)
{
	int x;

	for(x=0;x<value;x++){
		a++;
	}
}

/* Let's have a Software busy loop correlation with system time stamp
 * Autocalibrate is converging to 1msec delay*/
#define CONVERGE_RATE	64

unsigned long autocalibrating(void)
{
	unsigned long delay = 10;/*Start with something*/
	unsigned long speed,average,average_speed;
	char progress[4] = {'|','/','-','\\'};
	int state=0,y,gear;
	int marker;
	unsigned long up_convergence=CONVERGE_RATE;

	struct timeval prev, now;

	printf("Software Autocalibration loop\n");
	average =0;
 average_speed=0;
 y=0;
 gear=0;
 while(1){
  gettimeofday (&prev, NULL);
  sw_delay(delay);
  gettimeofday (&now, NULL);

  if(now.tv_usec > prev.tv_usec)
   speed =(now.tv_usec - prev.tv_usec);
  else
   speed =((TIMEVAL_USEC_WRAP-prev.tv_usec) + now.tv_usec);

  printf("\r\t%c",progress[state]);
  state++;
  if(state>=4)
   state=0;
  fflush(stdout);

  if(speed>1000){/*1Ms*/
   average += delay;
   average_speed +=speed;
   y++;
   if(y>2 && gear==0){/*Slow it down*/
    gear=1;
    up_convergence=CONVERGE_RATE>>1;
   }
   if(y>4 && gear==1){/*Slow it down*/
    gear=2;
    up_convergence=CONVERGE_RATE>>2;
   }
   if(y>8 && gear==2){/*Slow it down*/
    gear=3;
    up_convergence=CONVERGE_RATE>>3;
   }
   if(y>16 && gear==3){/*Slow it down*/
    gear=4;
    up_convergence=CONVERGE_RATE>>5;
   }
   if(y>30){
    printf("\n1msec delay %ld, average time %ld\n",average/y, average_speed/y);
    return(average/y);
   }
   printf("\r\t\t\t%8ld            %4d",delay,up_convergence);
  }
		else{
			y=0;
			average =0;
			average_speed=0;
			delay = delay+up_convergence;
		}

	}
}




  pid_t pid;
  char buf[200];

/*Take the WebOS PID And pass it to Linux IRQ driver (for signal SIGUSR1 NMI)*/
  pid = getpid();
  
  
  
  
  
#include <sys/signal.h>

static void
sig_handler(int sig)
{


int
init_sig(void)
{
    int sig;
    signal(SIGUSR1, sig_handler);
    return(1);
}

volatile int a=0;

void main(void)
{
	init_sig();
	
	
	
	
	
	
	
	
	
	
	
	
         case 'h':
	    printf("\n-------- HELP --------\n\n");
            printf("Syntax: \n");
            printf("hapictst [-h] | \n");
            printf("          -c pmem\n" );
            printf("          -l <pmem_label_number>\n" );
            printf("          -o <pmem_operation>\n" );
            printf("          (valid <pmem_operation>: get, put or query)\n\n" );
            printf("  (enter no parameters to do the regular hapictst)\n" );
	    printf("--- end of help -------\n");
            return 0;

         /* get category of the command */
         case 'c':
	 {                    
            if( optarg == NULL )
            {
               printf( "Error: No valid category specified (use -h for help)\n" );
               return -1;
            }

            category = (string)optarg;  // save the operation parameter string

            // only pmem test functions are input via the command line parameters at this time.
            // (go ahead and add new ones as you need them)
            if (category !=  "pmem")   
            {
               printf("Error: pmem is the only valid category at this time (use -h for help)\n");
            }

            break;
            
            
            
            
            


#if 0

/*Factor is the ops operand*/
/*ops*/
#define MULTIPLY 1
#define DIVIDE 0
/*Zone*/
#define ZONE_MIN  0x1
#define ZONE_LOW 0x2
#define ZONE_HIGH 0x4

#define FACTOR 2
void change_zone_balance(int factor, int ops, int zone_mask)
{
 int j;
 pg_data_t *tmpdat = pgdat_list;

 for (j = 0; j < MAX_NR_ZONES; j++) {
  zone_t *zone = tmpdat->node_zones + j;
                if(ops == DIVIDE){
                 if(zone_mask & ZONE_MIN)
    zone->pages_min = zone->pages_min>>factor;
       if(zone_mask & ZONE_LOW)
    zone->pages_low = zone->pages_low>>factor;
       if(zone_mask & ZONE_HIGH)
                  zone->pages_high = zone->pages_high>>factor;
                }
                if(ops == MULTIPLY){
                 if(zone_mask & ZONE_MIN)
				zone->pages_min = zone->pages_min<<factor;
    			if(zone_mask & ZONE_LOW)
				zone->pages_low = zone->pages_low<<factor;
    			if(zone_mask & ZONE_HIGH)
                		zone->pages_high = zone->pages_high<<factor;
                }
	}
}
	/*Adjust to High water mark and see if we are still in the "balanced" state*/
	change_zone_balance(FACTOR, MULTIPLY,ZONE_HIGH);

        /*Don't do anything if cranked zone are imbalanced*/
        if(!is_all_zone_balanced()){
		release_initial_zone_setting();
        	return 1;
	}

	if(prime_emergency_pool()){
        	release_initial_zone_setting();
        	printk("\nCannot Prime the buffer at Init TIme!!!\n");
                return 1;
	}
#endif
/*
 * CONFIG_OOM_EMERGENCY:
 * When kswapd receive the signal SIGUSR1 this mean that kswapd has to:
 * - shrink the cache to the maximum value
 * - If an emergency pool value is defined by the /proc entry then try to allocate it.
 * - Report the status of MM with a /proc entry right after the above operation.
 */

#define KSWAPD_SIGNAL SIGUSR1

struct emergency_pool_req_t emergency_pool_req;
struct emergency_pool_status_t emergency_pool_status;

static spinlock_t oom_emergency_lock = SPIN_LOCK_UNLOCKED;
static LIST_HEAD(oom_emergency_pages);

static pid_t critical_pid=0;/*Used to deliver that emergency signal*/
static int nr_oom_emergency_pages;
static int nr_total_oom_emergency_pages;

int oom_compress_cache=0;

static void manual_kswapd_balance(void);

static void stats_oom_emergency_pool(void)
{
 #define B(x) ((unsigned long long)(x) << PAGE_SHIFT)
        struct emergency_pool_status_t *status = &emergency_pool_status;
 struct sysinfo i;
 int pg_size ;

 si_meminfo(&i);
 si_swapinfo(&i);
 pg_size = atomic_read(&page_cache_size) - i.bufferram ;

        /*Don't know if it's revalent???????????????*/
 status->used = B(i.totalram-i.freeram);
 status->free = B(i.freeram);
 status->cached = B(pg_size);

 status->emergency_buf  = B(nr_total_oom_emergency_pages);
 status->emergency_buf_free = B(nr_oom_emergency_pages);
}

static void init_oom_emergency_pool(int pg_size)
{
 struct sysinfo i;

        manual_kswapd_balance();/*Here I assume that the page_cache has been shrinked otherwise freeram is going to be Really small!*/

 si_meminfo(&i);
        if(pg_size > ((i.freeram * 10)/100)){
         printk("Warning exceeding maximum ratio\n");
  pg_size=((i.freeram * 10)/100);
 }

        printk("init_oom_emergency_pool pg_size=%ld i.freeram=%d\n",pg_size,(i.freeram));

	spin_lock_irq(&oom_emergency_lock);
	while (nr_oom_emergency_pages < pg_size) {
		struct page * page = alloc_page(GFP_HIGHUSER);
		if (!page) {
			printk("couldn't refill oom emergency pages");
			break;
		}
		list_add(&page->list, &oom_emergency_pages);
		nr_oom_emergency_pages++;
	}
	spin_unlock_irq(&oom_emergency_lock);

        /*Mark the total amount of allocated page*/
	nr_total_oom_emergency_pages=nr_oom_emergency_pages;
	printk("allocated %d pages\n",nr_total_oom_emergency_pages);

        /*Generate stats entry*/
        stats_oom_emergency_pool();
}

/*
 * Cache shrink failed. Normally this would result in a OOM situation.
 * Here we have an emergency buffer that behave like system cache so let's try to shrink it...
 * Registered application has to be notified...
 * It's possible to exhausted the emergency pool itself. In that case nothing is really different than
 * the usual OOM situation.
 * --etiennem
 *
 * nr_pages == 0 -> Shrink all oom buffer
 * return the number of pages that hasn't been freed, 0 -> request has been fully process
 */
int shrink_oom_emergency_pool(int nr_pages)
{
	struct list_head *tmp;
	struct page *page;
	int clean=0;
        static int flag=0;

        if(!nr_pages)/*shrink all*/
        	clean=1;

	if(!flag){/*Let's signal the apps only the first time*/
		//A bit of checking for the signal to be delivered...*/
        	//Raise signal to registered APPS
		flag=1;
	}

	tmp = &oom_emergency_pages;
	spin_lock_irq(&oom_emergency_lock);
        if(clean)
        	nr_pages = nr_oom_emergency_pages;
        while(nr_pages){
		if (!list_empty(tmp)) {
			page = list_entry(tmp->next, struct page, list);
			list_del(tmp->next);
			nr_oom_emergency_pages--;
               		nr_pages--;
   if(page)
    __free_page(page);
  }
  else
                 break;
 }
  BUG_ON(clean && nr_pages); /*Should always be able to clean to whole list*/

        if(!nr_pages)/*Mark the total amount of page*/
  nr_total_oom_emergency_pages=nr_oom_emergency_pages;

        stats_oom_emergency_pool();

        return nr_pages;
}

void register_oom_signal(struct task_struct *tsk )
{
 spin_lock_irq(&tsk->sigmask_lock);
 sigfillset(&tsk->blocked);
 siginitsetinv(&current->blocked, sigmask(KSWAPD_SIGNAL));
 recalc_sigpending(tsk);
 spin_unlock_irq(&tsk->sigmask_lock);
}

int check_oom_signal(struct task_struct *tsk )
{
 int value=0;
        struct emergency_pool_req_t *req = &emergency_pool_req;
        struct emergency_pool_status_t *status = &emergency_pool_status;
        struct task_struct *p =NULL;

 if (signal_pending(tsk)) {
  spin_lock_irq(&tsk->sigmask_lock);
  if (sigismember(&tsk->pending.signal, SIGUSR1)) {
   sigdelset(&tsk->pending.signal, SIGUSR1);
   value=1;
  }
  recalc_sigpending(tsk);
  spin_unlock_irq(&tsk->sigmask_lock);
 }
/*Think about the problem where SWMON restart... Multiple init.
So allow multiple init, simply change the PID to the new request & make sure size match!!!*/

        if(value){
		if(req->active_flag && !status->emergency_buf){/*An emergency pool is requested*/
			p = find_task_by_pid(req->pid);  /* identify target of query */
			if((p && !strcmp(p->comm, "swmon")) || (req->active_flag == 12345)){/*Only swmon can request emergency pool & the bypass flag*/
				init_oom_emergency_pool(req->pg_size);
                        	critical_pid = req->pid;
			}
		}
		if(!req->active_flag && status->emergency_buf){/*Destroy the emergency pool*/
			shrink_oom_emergency_pool(0);
                	critical_pid=0;
		}
        }
        return value;
}

static int manual_kswapd_balance_pgdat(pg_data_t * pgdat)
{
	int need_more_balance = 0, i;
	zone_t * zone;

	for (i = pgdat->nr_zones-1; i >= 0; i--) {
		zone = pgdat->node_zones + i;
		if (unlikely(current->need_resched))
			schedule();
		if (!try_to_free_pages_zone(zone, GFP_KSWAPD)) {
			continue;
		}
                /*If try_to_free_pages_zone==0  for all zone then the max shrink level is reached.*/
                need_more_balance = 1;
	}
	return need_more_balance;
}

static void manual_kswapd_balance(void)
{
	int need_more_balance;
	pg_data_t * pgdat;

        oom_compress_cache=1;
	do {
		need_more_balance = 0;

		for_each_pgdat(pgdat)
			need_more_balance |= manual_kswapd_balance_pgdat(pgdat);
	} while (need_more_balance);
        oom_compress_cache=0;
}
#endif

	


int
main ( int argc, char **argv )
{
  int rc = 1;
  int opt;
  char *endPtr;
  const char *short_options = "g:s:e:a:vVh?";
  int options_index = 0;
  static struct option long_options[] =
  {
    {"get", 1, 0, 'g' },
    {"set", 1, 0, 'p' },
    {"exec", 1, 0, 'e' },
    {"affinity", 1, 0, 'a' },
    {"verbose", 0, 0, 'v' },
    {"moreverbose", 0, 0, 'V' },
    {"help", 0, 0, 'h' },
    {0, 0, 0, 0}
  };
  Command command = c_COUNT;
  unsigned long long inputAffinity = 0;
  pid_t pid = 0;
  char *execCmd = NULL;
  char **execArgv = NULL;
  int execArgc = 0;
  cpu_set_t cpuAffinity;

  CPU_ZERO(&cpuAffinity);

  while( 1 )
  {
    opt = getopt_long( argc, argv, short_options,
                       long_options, &options_index );
    if( opt < 0 )
      break;

    switch( opt )
    {
      case 'a':
        inputAffinity = strtoull( optarg, &endPtr, 2 );
        if( *endPtr || inputAffinity == 0 )
        {


void
syntax ( const char *program )
{
  fprintf( stderr,
    "Syntax: %s <-g <process> | <<-e <command> <args> | -s <process>> -a <affinity>>>\n"
    "Options: (-g, -s and -a, or -e and -a)\n"
    "  -g|--get <pid,\"parent\">\n"
    "    Get the process pid's (or the 'parent' of %s) affinity.\n"
    "  -s|--set <pid,\"parent\">\n"
    "    Set the process pid's (or the 'parent' of %s) affinity.\n"
    "  -e|--exec <command> <args>\n"
    "    Execute <command>. All remaining parameters will be used\n"
    "    as arguments to <command>.\n"
    "  -a|--affinity <affinity_binary>\n"
    "    Set the process's affinity (select a process with -p, -P, or -e).\n"
    "    Set the CPU affinity in binary (big endian). Examples:\n"
    "      - to use only CPU0: 1,\n"
    "      - to use only CPU1: 10,\n"
    "      - to use both CPU0 and CPU1: 11,\n"
    "      - to use CPU3, CPU1, and CPU0: 1011.\n"
    "  -v|--verbose\n"
    "    Be verbose.\n"
    "  -V|--moreverbose\n"
    "    Be really verbose.\n"
    "  -h|-?|--help\n"
    "    Print this text.\n",
    program, program, program );
}	

int main(int argc, char *argv[])
{
        char *cmd, *end;
        int cmd_length,tmp;
	char *name = argv[0];


	while(argc){
		argv++;
		argc--;

		for ( ; argc; argc--, argv++){

                	cmd = "minor=";
                        cmd_length = strlen(cmd);
			if ( strncmp( cmd, *argv, cmd_length ) == 0 ){
				mem_ctrl_st.minor= strtoul( (*argv)+cmd_length,0,0);
				continue;
			}

			tmp = strtoul( *argv, &end, 0);
			if ( *end ){
				fprintf(stderr,"Unrecognized keyword: %s\n", *argv );
                                usage(name);
				exit(1);
			}

			if ( end == *argv )
				continue;
		}
	}
}



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

static int printf_debug =0;
#define PRINTF			if (printf_debug) printf

void die(char *msg,...)
{
    va_list args;

    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
    fprintf(stderr,"\n");
    exit(1);
}

struct memory_ctrl{
	int minor;
	int major;
	int critical;
	int oom;
	char log_file[25];
	char x_cmd[100];
};

/*By default no log_file, no external command, no OOM signal*/
static struct memory_ctrl mem_ctrl_st ={150,125,100,0,NULL,NULL};

static void usage(char *name)
{
	fprintf(stderr,"\n-------- HELP --------\n");
	fprintf(stderr,"Syntax: \n");
	fprintf(stderr,"%s \n",name);
	fprintf(stderr,"          -m <minor>\n" );
	fprintf(stderr,"          -M <major>\n" );
	fprintf(stderr,"          -c <critical>\n" );
	fprintf(stderr,"          -o <oom>\n" );
	fprintf(stderr,"          -l <log_file>\n" );
	fprintf(stderr,"          -x <x_cmd>\n" );
	fprintf(stderr,"--- end of help -------\n");
}

int main(int argc, char *argv[])
{
	char *name = argv[0];
	char *tmp;
	int c;

	/* have to be reset in order for getopt() to work properly (not documented)  */
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;

	while ((c = getopt (argc, argv, "hvm:M:c:o:l:x:")) != EOF){
		/* Scan the command line for options */
		switch (c) {
			case 'm':
			mem_ctrl_st.minor = atoi((const char*)optarg);
			if (mem_ctrl_st.minor<10 || mem_ctrl_st.minor>1000){
				fprintf(stderr,"Invalid minor value %d\n",mem_ctrl_st.minor);
				return -1;
			}
			break;

			case 'M':
			mem_ctrl_st.major = atoi((const char*)optarg);
			if (mem_ctrl_st.major<10 || mem_ctrl_st.major>1000){
				fprintf(stderr,"Invalid major value %d\n",mem_ctrl_st.major);
				return -1;
			}
			break;

			case 'c':
   mem_ctrl_st.critical = atoi((const char*)optarg);
   if (mem_ctrl_st.critical<10 || mem_ctrl_st.critical>1000){
    fprintf(stderr,"Invalid critical value %d\n",mem_ctrl_st.critical);
    return -1;
   }
   break;

   case 'o':
   mem_ctrl_st.oom = atoi((const char*)optarg);
   if (mem_ctrl_st.oom<1 || mem_ctrl_st.oom>100){
    fprintf(stderr,"Invalid oom value %d\n",mem_ctrl_st.oom);
    return -1;
   }
   break;
   
   case 'l':
   strncpy (mem_ctrl_st.log_file,(const char*)optarg,sizeof(mem_ctrl_st.log_file));
   break;

   case 'x':
   strncpy (mem_ctrl_st.x_cmd,(const char*)optarg,sizeof(mem_ctrl_st.x_cmd));
   break;

   case 'h':
    usage(name);
    return 1;
   break;

   case 'v':
    printf_debug=1;
    PRINTF("Verbose On\n");
   break;

   /* unknown options */
   case '?':
   if( isprint( optopt ) )
    fprintf(stderr, "Unknown option '-%c'.\n", optopt );
   else
    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt );
   return 1;
   break;

       default:
    die("parser error\n");
   break;
		}
	}

	/* print out the invalid, non-option args that were ignored */
	{ /* narrow the scope */
		int index = 0;
		for( index = optind; index < argc; index++ )
		fprintf(stderr, "Non-option argument \"%s\"\n", argv[index] );
	}
	PRINTF("<minor> %d\n",mem_ctrl_st.minor );
	PRINTF("<major> %d\n",mem_ctrl_st.major );
	PRINTF("<critical> %d\n",mem_ctrl_st.critical );
	PRINTF("<oom> %d\n",mem_ctrl_st.oom );
	PRINTF("<log_file> %s\n",mem_ctrl_st.log_file );
	PRINTF("<x_cmd> %s\n",mem_ctrl_st.x_cmd );
}



/* Read/Write to/from MV64360 internal registers */
#define MV_REG_READ(offset) le32_to_cpu(* (volatile unsigned int *) (mv64360_base + offset))
#define MV_REG_WRITE(offset,data) *(volatile unsigned int *) (mv64360_base + offset) = cpu_to_le32 (data)
#define MV_SET_REG_BITS(regOffset,bits) ((*((volatile unsigned int*)((mv64360_base) + (regOffset)))) |= ((unsigned int)cpu_to_le32(bits)))
#define MV_RESET_REG_BITS(regOffset,bits) ((*((volatile unsigned int*)((mv64360_base) + (regOffset)))) &= ~((unsigned int)cpu_to_le32(bits)))





#include "kernel_debug.h"

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
#ifdef MIPS
extern unsigned long highmem_start_page;
#endif
#define SANITY_HIGHMEM(tmpptr,page) do{\
	if(!tmpptr)\
		PANIC("Cannot map HighMem page");\
}while(0)
/*	
	if (page < highmem_start_page)\
		PRINT("\nPage is not in HIGHMEM!!!");\
*/

#ifdef MIPS

static inline int pte_uncache(pte_t pte)
{
	return pte_val(pte) & _CACHE_UNCACHED;
}

static inline int pte_cache(pte_t pte)
{
	return pte_val(pte) & PAGE_CACHABLE_DEFAULT;
}
#endif


#define PRINT_PAGE_BIT(pte) do{\
	printk("Read ?%d\n",pte_read(pte) ?1:0);\
	printk("Write ?%d\n",pte_write(pte) ?1:0);\
	printk("Dirty ?%d\n",pte_dirty(pte) ?1:0);\
	printk("Young ?%d\n",pte_young(pte) ?1:0);\
	printk("Exec ?%d\n",pte_exec(pte) ?1:0);\
	printk("Uncache ?%d\n",pte_uncache(pte) ?1:0);\
	printk("Cache ?%d\n",pte_cache(pte) ?1:0);\
while(0)

static void display_page(struct page *page)
{
	char *tmpptr;
	int x;

	if (!VALID_PAGE(page))
		BUG();
	tmpptr =kmap(page);
	printk("Highmem ptr =%lx",(unsigned long)tmpptr);
	SANITY_HIGHMEM(tmpptr,page);
	printk("\n");
	for(x=0;x<PAGE_SIZE;x++){
		printk("%x",tmpptr[x]);
		if(!(x%16)){
			printk("\n");
		}
	}
	kunmap(page);
}

static void write_page(struct page *page,unsigned long value)
{
	char *tmpptr;

	if (!VALID_PAGE(page))
		BUG();
	tmpptr =kmap(page);
	printk("Highmem ptr =%lx",(unsigned long)tmpptr);
	SANITY_HIGHMEM(tmpptr,page);
	memset(tmpptr,value,PAGE_SIZE);
	//clean_dcache_range((u32)tmpptr,(u32)((u32)tmpptr+PAGE_SIZE));
	kunmap(page);
}

/*
 * address is aligned inside
 *
 */
static int vaddr_to_pte(unsigned long address,pte_t *pte,pid_t pid)
{
 pgd_t *dir;
 pmd_t *pmd;
 struct task_struct *taskptr;
 struct mm_struct *mm;

 printk("address = %lx\n",address);
 if (address >= TASK_SIZE){/*Kernel Addr range so get the swapper PGD*/
  mm = &init_mm;
  printk("Swapper PGD\n");
 }

 else{
  if(pid){
   taskptr = find_task_by_pid(pid);
   if(!taskptr){
    printk("Cannot find process %d",(unsigned long)pid);
    return 0;
   }
   mm = taskptr->mm;
   printk("PID %d PGD\n",(unsigned long)pid);
  }
  else{
   mm = current->mm;
   printk("Current PGD\n");
  }
 }

 dir = pgd_offset(mm, address & PAGE_MASK);
 if (dir) {
  pmd = pmd_offset(dir, address & PAGE_MASK);
  if (pmd && pmd_present(*pmd)) {
   pte = pte_offset(pmd, address & PAGE_MASK);
   if (pte && pte_present(*pte)) {
    return 1;
   }
  }
  else {
   return (0);
  }
 }
 else {
		return (0);
	}
	return (0);
}


/* The pgtable.h claims some functions generically exist, but I
 * can't find them......
 */
pte_t *my_va_to_pte(unsigned long address,pid_t pid)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte;
	struct mm_struct *mm;
	struct task_struct *taskptr;

	if (address < TASK_SIZE){
		if(pid){
			printk("PID = %d\n",pid);
			taskptr = find_task_by_pid(pid);
			if(!taskptr){
				printk("Cannot find process %d",(unsigned long)pid);
				return 0;
			}
			mm = taskptr->mm;
		}
		else{
			printk("Current\n");
			mm = current->mm;
		}
	}
	else{
		printk("Swapper\n");
		mm = &init_mm;
	}

	dir = pgd_offset(mm, address & PAGE_MASK);
	if (dir) {
		pmd = pmd_offset(dir, address & PAGE_MASK);
		if (pmd && pmd_present(*pmd)) {
			pte = pte_offset(pmd, address & PAGE_MASK);
			if (pte && pte_present(*pte)) {
				return(pte);
			}
		}
		else {
			return (0);
		}
	}
	else {
		return (0);
	}
	return (0);
}

/*
 * HighMem interface
 */
int do_memhigh(cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char *argv[])
{
	unsigned long paddr,value;
	struct page *page;
	char *tmpptr;
	int x;

	CHECK_ARG_NUM(argc,3);

	if(!strcmp(argv[1], "rdpaddr")){/*Read Physical addr even if in HighMem*/
		sscanf(argv[2],"%lx",&paddr);
		printk("rdpaddr -> %lx\n",paddr);
		page =  (struct page*)(mem_map+(paddr>>PAGE_SHIFT));
		display_page(page);
	}

	if(!strcmp(argv[1], "wrpaddr")){/*Write Physical addr even if in HighMem*/
		CHECK_ARG_NUM(argc,4);
		sscanf(argv[2],"%lx",&paddr);
		sscanf(argv[3],"%lx",&value);
		printk("wrpaddr -> %lx=%x\n",paddr,value);
		page =  (struct page*)(mem_map+(paddr>>PAGE_SHIFT));
		write_page(page,value);
	}
	return 0;
#if 0

	if(!strcmp(token_array[0].element, "rdvaddr")){/*???*/
  sscanf(token_array[1].element,"%lx",&vaddr);
  printk("rdvaddr -> %lx\n",vaddr);
  page = virt_to_page(vaddr);
  if (!VALID_PAGE(page))
   BUG();
  tmpptr =kmap(page);
  printk("Highmem ptr =%lx",(unsigned long)tmpptr);
  SANITY_HIGHMEM(tmpptr,page);
  for(x=0;x<PAGE_SIZE;x++){
   printk("%x",tmpptr[x]);
   if(!(x%16)){
    printk("\n");
   }
  }
  kunmap(page);
 }
#endif
}


/*
 * Cache interface
 */
int do_memcache(cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char *argv[])
{
 unsigned long address;
 pte_t *pte;
 u32 firstPage;
 u32 addr;
 pid_t pid;

 CHECK_ARG_NUM(argc,3);

 if(!strcmp(argv[1], "get_pte")){/*With a vaddr, get page and get pte bit setting*/
  sscanf(argv[2],"%lx",&address);
  pte = my_va_to_pte(address,NULL);
  if(pte){
   printk("pte_val(x) =%lx\n",pte_val(*pte));
   printk("pte page addr = %lx\n",((unsigned long)(pte_val(*pte)) & PAGE_MASK) | (address & ~(PAGE_MASK)));
   /*Ok if this a Kernel PTE then the Vaddr->Paddr is linear...*/
   printk("Linear trans %lx -> %lx\n",address,__pa(address));
   PRINT_PAGE_BIT(*pte);
  }
  else
			printk("get_pte Invalid PTE\n");
	}

	if(!strcmp(argv[1], "get_dpte")){/*With a vaddr, get page and get pte bit setting*/
		sscanf(argv[2],"%lx",&address);
		sscanf(argv[3],"%d",&pid);
		pte = my_va_to_pte(address,pid);
		if(pte){
			printk("pte_val(x) =%lx\n",pte_val(*pte));
			printk("pte page addr = %lx\n",((unsigned long)(pte_val(*pte)) & PAGE_MASK) | (address & ~(PAGE_MASK)));
			/*Ok if this a Kernel PTE then the Vaddr->Paddr is linear...*/
			printk("Linear trans %lx -> %lx\n",address,__pa(address));
			PRINT_PAGE_BIT(*pte);
		}
		else
			printk("get_pte Invalid PTE\n");
	}

	if(!strcmp(argv[1], "new_kpte")){/*On Mips the Kernel is not PTE mapped so no translation*/
		address = kmalloc((1 * PAGE_SIZE), GFP_KERNEL);
  printk("address = %lx\n",address);
  pte = my_va_to_pte(address,NULL);
  if(pte){
   printk("pte_val(x) =%lx\n",pte_val(*pte));
   printk("pte page addr = %lx\n",((unsigned long)(pte_val(*pte)) & PAGE_MASK) | (address & ~(PAGE_MASK)));
   /*Ok if this a Kernel PTE then the Vaddr->Paddr is linear...*/
   printk("Linear trans %lx -> %lx\n",address,__pa(address));
   PRINT_PAGE_BIT(*pte);
  }
  else
   printk("new_pte Invalid PTE\n");
 }

 if(!strcmp(argv[1], "new_upte")){
  address = __get_free_pages(GFP_HIGHUSER, 1);
  printk("address = %lx\n",address);
  pte = my_va_to_pte(address,NULL);
  if(pte){
   printk("pte_val(x) =%lx\n",pte_val(*pte));
   printk("pte page addr = %lx\n",((unsigned long)(pte_val(*pte)) & PAGE_MASK) | (address & ~(PAGE_MASK)));
   /*Ok if this a Kernel PTE then the Vaddr->Paddr is linear...*/
   printk("Linear trans %lx -> %lx\n",address,__pa(address));
   PRINT_PAGE_BIT(*pte);
  }
  else
   printk("new_pte Invalid PTE\n");
 }

}

/*
 * HighMem interface
 */
extern void internal_memq_query(pid_t pid,unsigned long addr,struct memq_info_t *info);
/*
echo "mem_query  135000 696 ">kernel_debug
*/
struct memq_info_t
 {
 unsigned long  a_start;
 unsigned long  a_end;
 int   flags;
 unsigned long  pte;
 unsigned long  offset;
 ino_t   inode;
 dev_t   dev;
 char   *path;
 };

int do_memquery(cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char *argv[])
{
 unsigned long paddr,value;
 struct page *page;
 char *tmpptr;
 int x;
 pid_t pid;
 struct memq_info_t ki;

 CHECK_ARG_NUM(argc,3);

 sscanf(argv[1],"%lx",&paddr);
// printk("Addr -> %lx\n",paddr);

 sscanf(argv[2],"%ld",&pid);
// printk("Addr -> %ld\n",pid);
 
 internal_memq_query(pid,paddr,&ki);

}

#if 0

 if(!strcmp(argv[1], "get_pte")){/*With a vaddr, get page and get pte bit setting*/
  sscanf(argv[2],"%lx",&vaddr);
  if(vaddr_to_pte(vaddr,&pte,NULL)){
   printk("Invalid PTE\n");
   return 0;
  }
  /*Ok ket's get some info about that pte*/
  printk("pte_val(x) =%lx\n",pte_val(pte));
  printk("pte page addr = %lx\n",pte_val(pte) & PAGE_MASK);
  printk("pte bit field = %lx\n",pte_val(pte) & ~PAGE_MASK);
  /*Ok if this a Kernel PTE then the Vaddr->Paddr is linear...*/
  printk("Linear trans %lx -> %lx\n",vaddr,__pa(vaddr));

  PRINT_PAGE_BIT(pte);
 }

 if(!strcmp(argv[1], "walk_user_pte")){/*With a vaddr, get page and get pte bit setting*/
  CHECK_ARG_NUM(argc,4);
  sscanf(argv[2],"%lx",&vaddr);
  vaddr=vaddr&PAGE_MASK;/*Align it*/
  if (vaddr >= TASK_SIZE){
   printk("Not a User PTE\n");
   return 0;
  }
  sscanf(argv[3],"%d",&pid);
  taskptr = find_task_by_pid(pid);
  if(!taskptr){
   printk("Cannot find process %d",(unsigned long)pid);
   return 0;
  }
  mm = taskptr->mm;
  if(vaddr_to_pte(vaddr,&pte,mm)){
   printk("Invalid PTE\n");
   return 0;
  }
  PRINT_PAGE_BIT(pte);
 }

 /*This function takes the a PID and a virtual addr and
 walk is PGD to get the PTE "the page frame in memory" + "the page attribute"*/
 if(!strcmp(argv[1], "virtual_to_pte")){
  CHECK_ARG_NUM(argc,4);
  sscanf(argv[2],"%lx",&vaddr);
  vaddr=vaddr&PAGE_MASK;/*Align it*/
  if (vaddr >= TASK_SIZE){
   printk("Not a User PTE\n");
   return 0;
  }
  sscanf(argv[3],"%d",&pid);
  taskptr = find_task_by_pid(pid);
  if(!taskptr){
   printk("Cannot find process %d",(unsigned long)pid);
   return 0;
  }
  mm = taskptr->mm;
  if(vaddr_to_pte(vaddr,&pte,mm)){
   printk("Invalid PTE\n");
   return 0;
  }
  page = pte_page(pte);
  printk("Physical addr = %lx\n",(page-mem_map)<<PAGE_SHIFT);
  display_page(page);
  PRINT_PAGE_BIT(pte);
 }
#endif









KErnel PATCH
#include <linux/config.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/memq.h>
#include <linux/miscdevice.h>

#include <asm/semaphore.h>
#include <asm/uaccess.h>


//static void __call_console_drivers(unsigned long start, unsigned long end);

typedef unsigned long (*fcnptr1)(const char *fmt, ...);
typedef void (*fcnptr)(unsigned long,unsigned long);
#if 0
 /* System.map
  * 9002cde4 T do_mmap_pgoff_mm
  */

 #define FCNPTR_DO_MMAP_PGOFF  0x9002cde4

       volatile fcnptr bin_entry;

       bin_entry = (fcnptr)FCNPTR_DO_MMAP_PGOFF;
       return ((*bin_entry)(current->mm,file,addr,len,prot,flags,pgoff));
#endif


extern void __call_console_drivers(unsigned long start, unsigned long end);


#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/smp_lock.h>
#include <linux/console.h>
void my__call_console_drivers(unsigned long start, unsigned long end)
{
 /*struct console *con;

 for (con = console_drivers; con; con = con->next) {
  if ((con->flags & CON_ENABLED) && con->write)
   con->write(con, &LOG_BUF(start), end - start);
 }*/
}


static int __init
BusLogic_Setup(char *str)
{
#if 0
 fcnptr  tmpptr;
 tmpptr  = (fcnptr)__call_console_drivers;

/*
        int ints[3];

        (void)get_options(str, ARRAY_SIZE(ints), ints);
*/

 printk("\nBusLogic_Setup %s\n",str);
 printk("\nPrintk addr = %x\n",(unsigned long)tmpptr);
#endif

#if 0
        if (ints[0] != 0) {
                BusLogic_Error("BusLogic: Obsolete Command Line Entry "
                                "Format Ignored\n", NULL);
                return 0;
        }
        if (str == NULL || *str == '\0')
                return 0;
        return BusLogic_ParseDriverOptions(str);
#endif
}

__setup("BusLogic=", BusLogic_Setup);



static int  memq_dev_init( void ){}
static void  memq_dev_exit( void ){}


module_init( memq_dev_init );
module_exit( memq_dev_exit );

MODULE_AUTHOR("Nortel Networks");
MODULE_DESCRIPTION("/dev interface to VM map");
MODULE_LICENSE("GPL");
#endif




PTR
Addr of ptr 1 bffff9a4

Addr of ptr 2 bffff9a0

Value of ptr 1 5

Value of ptr 2 4

Addr of ptr 1 8049ea8

Addr of ptr 2 8049d58

Value of myptr 1 5

Value of ptr 2 5

Addr of ptr 1 bffff9a4

Addr of ptr 2 bffff9a0

Value of ptr 1 5

Value of ptr 2 5

Addr of ptr 1 8049ea8

Addr of ptr 2 8049ea8
Aborted

int main(int argc, char *argv[])
{
 char *ptr1,*ptr2;
 unsigned long tmp;
 char **myptr;

 *ptr1 = 0x5;

 *ptr2 = 0x4;

 printf("\nAddr of ptr 1 %lx\n",(unsigned long)&ptr1);
	printf("\nAddr of ptr 2 %lx\n",(unsigned long)&ptr2);

	printf("\nValue of ptr 1 %lx\n",(unsigned long)*ptr1);
	printf("\nValue of ptr 2 %lx\n",(unsigned long)*ptr2);

	printf("\nAddr of ptr 1 %lx\n",(unsigned long)ptr1);
	printf("\nAddr of ptr 2 %lx\n",(unsigned long)ptr2);

	tmp =(unsigned long)(&(ptr1));	/*This is what we get from system map*/

	/*Now operate on tmp only*/
 myptr = (char**)tmp;
 //printf("\nValue of myptr 1 %lx\n",(unsigned long)*myptr);
 printf("\nValue of myptr 1 %lx\n",(unsigned long)**myptr);

 ptr2 = *myptr;
 printf("\nValue of ptr 2 %lx\n",(unsigned long)*ptr2);


 ptr2 =&(*ptr1);

 printf("\nAddr of ptr 1 %lx\n",(unsigned long)&ptr1);
 printf("\nAddr of ptr 2 %lx\n",(unsigned long)&ptr2);

 printf("\nValue of ptr 1 %lx\n",(unsigned long)*ptr1);
 printf("\nValue of ptr 2 %lx\n",(unsigned long)*ptr2);

 printf("\nAddr of ptr 1 %lx\n",(unsigned long)ptr1);
 printf("\nAddr of ptr 2 %lx\n",(unsigned long)ptr2);


 exit(0);
 
 
 
 
 
 
 
int oomprotect(int bytes);


//the actual implementation is below, but most people don't need to worry about this
_syscall2(int, oom_protect, pid_t, pid, unsigned long, max_vm)

inline int oomprotect(int bytes)
{
   int pages = bytes >> PAGE_SHIFT;
   return (int) oom_protect(getpid(), pages);          

}  


 for (ptr = strsep(&ptr1," "); ptr != NULL; ptr = strsep(&ptr1," ")){
  printk("\nSUB String %s\n",ptr);

       
		
		
#!/bin/bash

var0=0
LIMIT=10

while [ "$var0" -lt "$LIMIT" ]
do
  echo -n "$var0 "        # -n suppresses newline.
  #             ^           Space, to separate printed out numbers.

  var0=`expr $var0 + 1`   # var0=$(($var0+1))  also works.
                          # var0=$((var0 + 1)) also works.
                          # let "var0 += 1"    also works.
done                      # Various other methods also work.

echo

exit 0



fgrep ": 0 40 0 0" * |grep -v "0 30" |grep -v "0 49"

#!/bin/bash

var0=0
LIMIT=10000

while [ "$var0" -lt "$LIMIT" ]
do
  echo -n "$var0 "
  var0=`expr $var0 + 1`
done
exit 0


#!/bin/sh

var0=0
file="dump"
echo $1
while [ "$var0" -lt $1 ]
do
echo $file$var0

dd if=/dev/zero of=./trash/$file$var0 bs=1k count=1
var0=$(($var0+1))
done

exit 0
          

#!/bin/sh

var0=0
file="dump"
echo $1
while [ "$var0" -lt $1 ]
do
echo $file$var0

pmem_dump -p except   $file$var0
var0=$(($var0+1))
done

exit 0







/*******************************/
//   **Example of how to properly use the sigexit syscall**
//   author: Chris Friesen

// A few things to note:
//
// 1) we're using a realtime signal to get signal queueing
//    so that we don't drop signals that come in rapid succession
// 2) we're using a pipe to convert the async signal to
//    a synchronous message, thus letting us handle the
//    signal without having to worry about running in signal
//    handler context.
//
//  The program is run on the commandline with a single argument;
//  the pid of the process to monitor.  When that process dies,
//  this program will print out the information about the process
//  death.

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <fstream>
#include "sigexit.h"
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>


//Specify the signal that we will ask the kernel to send us
//Any signal between SIGRTMIN and SIGRTMAX is a valid realtime signal.
//The actual values of those defines are determined based on various
//factors such as whether or not the app is threaded.
#define WAKESIG SIGRTMIN+11

using namespace std;

int pipefds[2];

struct privatesiginfo
{
   int sig;
   int code;
   int status;
   pid_t pid;
};
      
void handler(int sig, siginfo_t *info, void *ptr)
{
   privatesiginfo pinfo;
   pinfo.sig = info->si_signo;
   pinfo.pid = info->si_pid;
   pinfo.status = info->si_status;
   pinfo.code = info->si_code;
   
   //store the errno so that we can restore it at the end of the
   //signal handler
   int temperrno = errno;
   
   //not much we can do if the write fails...it really shouldn't
   //any write of size under PIPE_BUF (4KB) is atomic
   write(pipefds[1], &pinfo, sizeof(pinfo));
   
   //restore the errno so that we don't confuse the mainline code
   errno = temperrno;
}


void userhandler()
{
   privatesiginfo pinfo;
   
   int rc;
   int len = sizeof(pinfo);
   char *ptr = (char*) &pinfo;
   
   //handle interrupted read() calls
   while (len > 0)
   { 
      rc = read(pipefds[0],ptr,len);
      if (rc < 0)
      {
         if (errno != EINTR)
            break;
      }
      else
      {
         ptr += rc;
         len -= rc;
      }
   }
   
   if (len != 0)
   {
      cout << "error, bad length on pipe read" << endl;
      return;
   }
      
   //yay, the read went okay
   cout << "got signal " << pinfo.sig << " from pid " << pinfo.pid << endl;
   cout << "code: " << hex << pinfo.code << " status: " << pinfo.status << endl;
}

int main(int argc, char **argv)
{
   int sig = WAKESIG;
   int rc = pipe(pipefds);
   if (rc < 0)
   {
      perror("pipe");
      return -1;
   }

   rc = fcntl(pipefds[0], F_SETFL, O_NONBLOCK);
   if (rc < 0)
   {
      perror("fcntl on read pipe");
      return -1;
   }
   
   rc = fcntl(pipefds[1], F_SETFL, O_NONBLOCK);
   if (rc < 0)
   {
      perror("fcntl on write pipe");
      return -1;
   }

   if ((argc <= 1) || (argc > 3))
   {
      printf ("Error: Provide pid and optional signal value.\n");
      return -1;
   }

   //should use strtol() in production code
   int pid = atoi(argv[1]);

   if (argc == 3)
   {
   //should use strtol() in production code
      sig = atoi(argv[2]); 
   }

   printf ("Test sigexit for pid %d and signal %d\n.",  pid, sig);  
 
   
   struct sigaction action;
   memset(&action, 0, sizeof(action));
   action.sa_sigaction = handler;
   action.sa_flags = SA_SIGINFO;
   sigaction(sig, &action, NULL);
      
   int sigexit_rc = sigexit(pid, sig);
   if (sigexit_rc < 0)
   {
      perror("sigexit");
      return -1;
   }
   
   fd_set readfds, workread;
   FD_ZERO(&readfds);
   FD_SET(pipefds[0], &readfds);
   
   while(1)
   {
      workread = readfds;
      rc = select(pipefds[0]+1, &workread, NULL, NULL, NULL);

      if (rc > 0)
      {
         if (FD_ISSET(pipefds[0], &workread))
               userhandler();
      }
   }

   return 0;
}
/****************************/     


static void *grab_file(const char *filename, unsigned long *size)
{
        unsigned int max = 16384;
        int ret, fd;
        void *buffer = malloc(max);

        if (streq(filename, "-"))
                fd = dup(STDIN_FILENO);
        else
                fd = open(filename, O_RDONLY, 0);

        if (fd < 0)
                return NULL;

        *size = 0;
        while ((ret = read(fd, buffer + *size, max - *size)) > 0) {
                *size += ret;
                if (*size == max)
                        buffer = realloc(buffer, max *= 2);
        }
        if (ret < 0) {
                free(buffer);
                buffer = NULL;
        }
        close(fd);
        return buffer;
}

int main(int argc, char *argv[])
{
        unsigned int i;
        long int ret;
        unsigned long len;
        void *file;
        char *filename, *options = strdup("");
        char *progname = argv[0];

        if (strstr(argv[0], "insmod.static"))
                try_old_version("insmod.static", argv);
        else
                try_old_version("insmod", argv);

        if (argv[1] && (streq(argv[1], "--version") || streq(argv[1], "-V"))) {
                puts(PACKAGE " version " VERSION);
                exit(0);
        }

        /* Ignore old options, for backwards compat. */
        while (argv[1] && (streq(argv[1], "-p")
                           || streq(argv[1], "-s")
                           || streq(argv[1], "-f"))) {
                argv++;
                argc--;
        }
                                           
 
 /*Adding element in a struture with a MACRO*/
#define SERIAL_PORT_DFNS  \
	       STD_UART_OP(0)		\
	       STD_UART_OP(1)
                 
#define STD_UART_OP(num)					\
	{ 0, BASE_BAUD, num, MPC85xx_IRQ_DUART,			\
		(ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST),	\
		iomem_base: (u8 *)MPC85XX_##num##_SERIAL,	\
		io_type: SERIAL_IO_MEM},
		
struct old_serial_port {
	unsigned int uart;
	unsigned int baud_base;
	unsigned int port;
	unsigned int irq;
	unsigned int flags;
	unsigned char hub6;
	unsigned char io_type;
	unsigned char *iomem_base;
	unsigned short iomem_reg_shift;
};



int
main(int argc, char **argv)
{
	struct module *mod;
	struct buffer buf = { };
	char fname[SZ];
	char *dump_read = NULL, *dump_write = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "i:mo:a")) != -1) {
		switch(opt) {
			case 'i':
				dump_read = optarg;
    break;
   case 'm':
    modversions = 1;
    break;
   case 'o':
    dump_write = optarg;
    break;
   case 'a':
    all_versions = 1;
    break;
   default:
    exit(1);
  }
 }  
 
 
 
# Step 2), invoke modpost
#  Includes step 3,4
quiet_cmd_modpost = MODPOST
      cmd_modpost = scripts/mod/modpost            \
        $(if $(CONFIG_MODVERSIONS),-m)             \
        $(if $(CONFIG_MODULE_SRCVERSION_ALL),-a,)  \
        $(if $(KBUILD_EXTMOD),-i,-o) $(symverfile) \
        $(filter-out FORCE,$^)

.PHONY: __modpost
__modpost: $(wildcard vmlinux) $(modules:.ko=.o) FORCE
        $(call cmd,modpost)             
 
 
 
 
void *
grab_file(const char *filename, unsigned long *size)
{
 struct stat st;
 void *map;
 int fd;

 fd = open(filename, O_RDONLY);
 if (fd < 0 || fstat(fd, &st) != 0)
  return NULL;

 *size = st.st_size;
 map = mmap(NULL, *size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
 close(fd);

 if (map == MAP_FAILED)
  return NULL;
 return map;
}



/*
 * Must define these before including other files, inline functions need them
 */
#define LOCK_SECTION_NAME                       \
        ".text.lock." __stringify(KBUILD_BASENAME)

#define LOCK_SECTION_START(extra)               \
        ".subsection 1\n\t"                     \
        extra                                   \
        ".ifndef " LOCK_SECTION_NAME "\n\t"     \
        LOCK_SECTION_NAME ":\n\t"               \
        ".endif\n"

#define LOCK_SECTION_END                        \
        ".previous\n\t"

#define __lockfunc fastcall __attribute__((section(".spinlock.text")))        




















/****************SWLOOP*//////////////////   
volatile unsigned long long incr_x;

void sw_loop(unsigned long long cal)
{
 local_irq_disable();
 printk("\nBefore sw_loop");
 for(incr_x=0; incr_x<cal; incr_x++){

 }
 printk("\nAfter sw_loop");
 local_irq_enable();

}

typedef void (*fcn_t)(unsigned long long ca);
fcn_t addr;
volatile unsigned long long cal=0;

void test(void)
{
 int z;
 unsigned long tmp;

 addr =  (fcn_t)sw_loop;
 printk("\nAddr = %lx",(unsigned long)addr);

       for(z=0;z<5;z++){
                tmp=jiffies;
                do{/*1 Sec Loop...*/
   cal++;
                }while((jiffies - tmp) <HZ);
                printk("\n%d Sec, cal %lx",z, cal);
        }

 /*Now let's do the sw loop*/
 sw_loop(cal);

}





#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int got_interrupt;

void intrup(int dummy) {
        got_interrupt = 1;
}

void die(char *s) {
        printf("%s\n", s);
        exit(1);
}

int main() {
        struct sigaction sa;
        int n;
        char c;

        sa.sa_handler = intrup;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGINT, &sa, NULL))
                die("sigaction-SIGINT");
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGQUIT, &sa, NULL))
                die("sigaction-SIGQUIT");

        got_interrupt = 0;
        n = read(0, &c, 1);
        if (n == -1 && errno == EINTR)
                printf("read call was interrupted\n");
        else if (got_interrupt)
                printf("read call was restarted\n");

        return 0;
}

Here Ctrl-C will interrupt the read call, while after Ctrl-\ the read call is restarted.

 c = console_drivers;
  c->write(c,"\nWarning Rx Buffer Overrun\n",strlen("\nWarning Rx Buffer Overrun\n"));
  static struct console *c = NULL;
  
  
  
  
  
  
  
static char preaction[16] = "pre_none";  
module_param_string(preaction, preaction, sizeof(preaction), 0);
MODULE_PARM_DESC(preaction, "Pretimeout action.  One of: "
   "pre_none, pre_smi, pre_nmi, pre_int.");
     
   
   
   
   
   
KPROBE Stuff:
static int count_generic_make_request = 0;

static int inst_generic_make_request(struct kprobe *p, struct pt_regs *regs)
{
 ++count_generic_make_request;
 return 0;
}

/*For each probe you need to allocate a kprobe structure*/
static struct kprobe kp = {
 .pre_handler = inst_generic_make_request,
 .post_handler = NULL,
 .fault_handler = NULL,
 .addr = (kprobe_opcode_t *) generic_make_request,
};

int init_module(void)
{
 register_kprobe(&kp);
 printk("kprobe registered\n");
 return 0;
}

void cleanup_module(void)
{
  unregister_kprobe(&kp);
  printk("kprobe unregistered\n");
  printk("generic_make_request() called %d times.\n",
  count_generic_make_request);
}

MODULE_LICENSE("GPL");
















VALIDATION AGAINST INPUT FIELD IN TEXT FILE

struct meminfo_t {
 char *name;
 int length;
 unsigned int *data;
} meminfo[] = { 
 { "MemTotal:",   9, &hoststat.mem[0] },
 { "MemUsed:",    8, &hoststat.mem[1] },
 { "MemFree:",    8, &hoststat.mem[2] },
 { "MemShared:", 10, &hoststat.mem[3] },
 { "Buffers:",    8, &hoststat.mem[4] },
 { "Cached:",     7, &hoststat.mem[5] },
 { "SwapTotal:", 10, &hoststat.swap[0] },
 { "SwapFree:",   9, &hoststat.swap[1] },
 { "SwapUsed:",   9, &hoststat.swap[2] },
 { NULL,          0, NULL }};



  while ((line = readline(stream)) != NULL) {
   n=skip(line);
   for (mp = meminfo; mp->name != NULL; mp++) {
    if (strncmp(line, mp->name, mp->length) == 0) {
     *(mp->data) = strtoul(n,&n,10);
    }
   }
  

   



struct used_car_common{
	char *milleage;
	char *year;
	char *phone;
};

struct model_st{
	char *model_0;
 char *model_1;
 char *model_2;
};

struct manuf_st{
 char *manuf;
 struct model_st *model;
};

struct manuf_st manuf_array[] ={
{  .manuf = "Honda",
  .model = (struct model_st[]){
   {
   .model_0 = "Civic",
   .model_1 = "Accord",
   .model_2 = "Crv",
   .model_3 = NULL,
   },
  },
},

{  .manuf = "Toyota",
  .model = (struct model_st[]){
   {
   .model_0 = "Corolla",
   .model_1 = NULL,
   },
  },

},

{  .manuf = "NULL",
},

};




Kernel DEBUG TRICK
#include <asm/time.h>
#include <asm/mpc85xx.h>
#include <asm/immap_85xx.h>
#include <asm/mmu.h>
#include <asm/ppc_sys.h>
#include <asm/kgdb.h>
#include <asm/machdep.h>
/*
 * Call the console drivers on a range of log_buf
 */
static void __call_console_drivers(unsigned long start, unsigned long end)
{
	struct console *con;

	char buffer[256];
	int len;

	memset(buffer,0,sizeof(buffer));
	len = end - start;
	if (len >256)
	 len=256;

	memcpy(buffer,&LOG_BUF(start), end - start);
	if (ppc_md.progress)
		ppc_md.progress(buffer, 0);


	for (con = console_drivers; con; con = con->next) {
		if ((con->flags & CON_ENABLED) && con->write &&
				(cpu_online(smp_processor_id()) ||
				(con->flags & CON_ANYTIME)))
			con->write(con, &LOG_BUF(start), end - start);
	}
}

/*
 * Write out chars from start to end - 1 inclusive
 */
static void _call_console_drivers(unsigned long start,
				unsigned long end, int msg_log_level)
{
	if (msg_log_level < console_loglevel &&
			console_drivers && start != end) {

	/*if (msg_log_level < console_loglevel && start != end) {*/
		if ((start & LOG_BUF_MASK) > (end & LOG_BUF_MASK)) {
			/* wrapped write */
			__call_console_drivers(start & LOG_BUF_MASK,
						log_buf_len);
			__call_console_drivers(0, end & LOG_BUF_MASK);
		} else {
			__call_console_drivers(start, end);
		}
	}
}


static int __init react_cmdline_wdt_enable(char *str)
{
	react_wdt_enabled=1;
	return 1;
}

device_initcall(react_wdt_init);

__setup("react_wdt", react_cmdline_wdt_enable);

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})




struct pci_bar_register{
	int offset;
	char* name;
	int tag;
};

struct pci_bar_register pci_bar_array[] = {
	{
		MV64340_PCI_DEVICE_AND_VENDOR_ID,
		"MV64340_PCI_DEVICE_AND_VENDOR_ID",
		0,
	},
	{
		MV64340_PCI_0_IO_BASE_ADDR,
		"MV64340_PCI_0_IO_BASE_ADDR",
		1,
	},

static int display_cfg (void)
{
	u32 val;
	int x;
	struct pci_bar_register *ptr;

	x=0;
	/*This is vendor/device ID*/
	ptr = &pci_bar_array[x];
	indirect_read_config(&hose, 0,ptr->offset,3,&val);
	printk("\n%s := %lx",ptr->name,(unsigned long)val);

	x=1;
	while(1){
		ptr = &pci_bar_array[x];
		if(!ptr->offset)
			break;



static int dump_cfg (void)
{
	int x;
	u32 val;
	while(1){
		indirect_read_config(&hose, 0,pci_dump_array[x],3,&val);
		printk("\n%x := %lx",pci_dump_array[x],(unsigned long)val);
		x++;
		if(pci_dump_array[x] == 0xdeedbeef)
			break;
	}
	return 0;
}


static int __init rtc_init(void)
{
	printk("\netienne");
	setup_indirect_pci(&hose,BRIDGE_BASE+MV64x60_PCI0_CONFIG_ADDR, BRIDGE_BASE+MV64x60_PCI0_CONFIG_DATA);
	display_cfg();
	return 1;	
}


static void __exit rtc_exit (void)
{

}

module_init(rtc_init);
module_exit(rtc_exit);

#ifdef CONFIG_PPC_INDIRECT_PCI_BE
#define PCI_CFG_OUT out_be32
#else
#define PCI_CFG_OUT out_le32
#endif

#define BRIDGE_BASE 0xf1000000
static struct pci_controller hose;

static int
indirect_read_config(struct pci_controller *hose, unsigned int devfn, int offset,
		     int len, u32 *val)
{
	volatile void __iomem *cfg_data;
	u8 cfg_type = 0;

//	printk("\nReading offset %x",offset);
	PCI_CFG_OUT(hose->cfg_addr,				 
		 (0x80000000 | ((0 - 0) << 16)
		  | (devfn << 8) | ((offset & 0xfc) | cfg_type)));

	/*
	 * Note: the caller has already checked that offset is
	 * suitably aligned and that len is 1, 2 or 4.
	 */
	cfg_data = hose->cfg_data + (offset );
	switch (len) {
	case 1:
		*val = in_8(cfg_data);
		break;
	case 2:
		*val = in_le16(cfg_data);
		break;
	default:
		*val = in_le32(cfg_data);
		break;
	}
	return PCIBIOS_SUCCESSFUL;
}

static int
indirect_write_config(struct pci_controller *hose, unsigned int devfn, int offset,
		      int len, u32 val)
{
	volatile void __iomem *cfg_data;
	u8 cfg_type = 0;


	PCI_CFG_OUT(hose->cfg_addr, 					 
		 (0x80000000 | ((0 - 0) << 16)
		  | (devfn << 8) | ((offset & 0xfc) | cfg_type)));

	/*
	 * Note: the caller has already checked that offset is
	 * suitably aligned and that len is 1, 2 or 4.
	 */
	cfg_data = hose->cfg_data + (offset );
	switch (len) {
	case 1:
		out_8(cfg_data, val);
		break;
	case 2:
		out_le16(cfg_data, val);
		break;
	default:
		out_le32(cfg_data, val);
		break;
	}
	return PCIBIOS_SUCCESSFUL;
}

void 
setup_indirect_pci_nomap(struct pci_controller* hose, void __iomem * cfg_addr,
	void __iomem * cfg_data)
{
	hose->cfg_addr = cfg_addr;
	hose->cfg_data = cfg_data;

}

void 
setup_indirect_pci(struct pci_controller* hose, u32 cfg_addr, u32 cfg_data)
{
	unsigned long base = cfg_addr & PAGE_MASK;
	void __iomem *mbase, *addr, *data;

	mbase = ioremap(base, PAGE_SIZE);
	addr = mbase + (cfg_addr & ~PAGE_MASK);
	if ((cfg_data & PAGE_MASK) != base)
		mbase = ioremap(cfg_data & PAGE_MASK, PAGE_SIZE);
	data = mbase + (cfg_data & ~PAGE_MASK);
	setup_indirect_pci_nomap(hose, addr, data);
}












+#undef VERY_EARLY_PRINTK
+#ifdef VERY_EARLY_PRINTK
+#include <asm/machdep.h>
+#endif
+
 /*
  * Call the console drivers on a range of log_buf
  */
 static void __call_console_drivers(unsigned long start, unsigned long end)
 {
+#ifndef VERY_EARLY_PRINTK
        struct console *con;

        for (con = console_drivers; con; con = con->next) {
@@ -333,16 +339,29 @@
                                (con->flags & CON_ANYTIME)))
                        con->write(con, &LOG_BUF(start), end - start);
        }
+#else
+       char buf[256];
+       memset(buf,0,256);
+       memcpy(buf,&LOG_BUF(start),end - start);
+       if (ppc_md.progress)
+               ppc_md.progress(buf, 0x123);
+#endif
 }

+
 /*
  * Write out chars from start to end - 1 inclusive
  */
 static void _call_console_drivers(unsigned long start,
                                unsigned long end, int msg_log_level)
 {
+#ifndef VERY_EARLY_PRINTK
        if (msg_log_level < console_loglevel &&
                        console_drivers && start != end) {
+#else
+       if (msg_log_level < console_loglevel &&
+                       start != end) {
+#endif
                if ((start & LOG_BUF_MASK) > (end & LOG_BUF_MASK)) {
                        /* wrapped write */









From bash and using custom lib patch driver:

export LD_LIBRARY_PATH=../../lib


Libraries have been installed in:
   /usr/local/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the `-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the `LD_RUN_PATH' environment variable
     during linking
   - use the `-Wl,--rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to `/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.


/*READING PORT DIRECTLY*/


dd if=/dev/port of=test bs=1 count=100 skip=4094




#define DEV_MEM "/dev/mem"
#define FatalError printf
#define BUS_BASE (0)
#define VIDMEM_READONLY             0x20 

void *
mapVidMem(int ScreenNum, unsigned long Base, unsigned long Size, int flags)
{
    void *base;
    int fd;
    int mapflags = MAP_SHARED; 
    int prot;
    unsigned long realBase, alignOff;

    realBase = Base & ~(getpagesize() - 1);
    alignOff = Base - realBase;
#ifndef MAP_NONCACHED
#define MAP_NONCACHED 0x00020000
#endif
        mapflags |= MAP_NONCACHED; 
    fd = open(DEV_MEM, (flags & VIDMEM_READONLY) ? O_RDONLY : O_RDWR);

    if (fd < 0)
    {
	FatalError("xf86MapVidMem: failed to open " DEV_MEM " (%s)\n",
		   strerror(errno));
    }

    if (flags & VIDMEM_READONLY)
	prot = PROT_READ;
    else
	prot = PROT_READ | PROT_WRITE;

    /* This requires linux-0.99.pl10 or above */
    base = mmap((caddr_t)0, Size + alignOff, prot, mapflags, fd,
 		(off_t)realBase  + BUS_BASE);
    close(fd);
    if (base == MAP_FAILED) {
        FatalError("xf86MapVidMem: Could not mmap framebuffer"
		   " (0x%08lx,0x%lx) (%s)\n", Base, Size,
		   strerror(errno));
    }
#ifdef DEBUG
    printf("base: %lx aligned base: %lx\n",base, base + alignOff);
#endif
    return (char *)base + alignOff;

}

int main(void)
{
	char *map;
	int x;
	map = (char*)mapVidMem(0, 0x80000000, 1024*1024, O_RDWR);


	for(x=0;x<1024*1024;x++){
		map[x] = map[x]-1;
		//printf("%x\n",map[x]);
	}

	return 0;
}









/usr/lib/vmware/lib/wrapper-gtk24.sh /usr/lib/vmware/lib /usr/lib/vmware/bin/vmware /usr/lib/vmware/libconf

