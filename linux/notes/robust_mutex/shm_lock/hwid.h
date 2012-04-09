/*
 * PCI topology path support and transparent pass-through
 *
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This code is licensed under the GNU GPL v2.
 */

#ifndef _HWID_H_
#define _HWID_H_

/*
 * WARNING
 * This file is shared across QEMU / Kernel
 *
 * TODO To be optimized; Currently based on linear search
 */

#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256
#endif

#define VENDOR 0x1af4
#define DEVICE 0x1234
#define MAX_PASSTHROUGH_DEV 256
#define HWID_TABLE_SIZE (1<<20) /* 1Mb */

enum ivshmem_registers {
    HWID = 0, /* UMD hp attach */
	HWID_STATUS = 4, /* UMD MSI status */
};

struct vbdf_lookup{
	int lock;
	
	/* Guest B:D.F */
	int g_busnr;
	int g_devnr;
	int g_fcnnr;

	/* Host B:D.F */
	int h_busnr;
	int h_devnr;
	int h_fcnnr;

	/* hwid topology path */
	char hwid[_POSIX_PATH_MAX];

	/* PCI error code */
	int err_code;
	int err_state;
	int err_code_ack;
	int err_result;

	/* Opaque */
	void *opaque;
};

struct hwid_req{
	int lock;
	int rc;
	/* hwid topology path */
	char hwid[_POSIX_PATH_MAX];
};

struct hwid_table{
	/* Lookup information */
	struct vbdf_lookup lookup[MAX_PASSTHROUGH_DEV];
	/* User mode driver generated */
	struct hwid_req req;
};

#ifdef __KERNEL__

static inline int get_irq_status(void *ptr)
{
	void __iomem *reg=ptr+HWID_STATUS;
	return readl(reg);
}

static inline void signal_irq_status(void *ptr, int slot)
{
	void __iomem *reg=ptr+HWID_STATUS;
	writel(slot,reg);
}

static inline void signal_hwid_register(void *ptr, int ops)
{
	void __iomem *reg=ptr+HWID;
	writel(ops,reg);
}

static inline int insert_hwid_request(void* ptr, char *hwid)
{
	struct hwid_req *r = &((struct hwid_table *)ptr)->req;
	strncpy(r->hwid,hwid,_POSIX_PATH_MAX);
	return ++r->lock;
}

static inline int get_hwid_request_number(void* ptr, int *rc)
{
	struct hwid_req *r = &((struct hwid_table *)ptr)->req;
	*rc = r->rc;
	return r->lock;
}

static inline char* vbdf_to_hwid(void *ptr,
	int gbusnr,	int gdevnr,	int gfcnnr)
{
	int nb_elem;
	struct vbdf_lookup *lookup = ((struct hwid_table *)ptr)->lookup;
		
	for(nb_elem=0; nb_elem < MAX_PASSTHROUGH_DEV; nb_elem++){
		if(!lookup[nb_elem].lock)
			continue;
		if( lookup[nb_elem].g_busnr == gbusnr &&
			lookup[nb_elem].g_devnr == gdevnr && 
			lookup[nb_elem].g_fcnnr == gfcnnr){
			break;
		}
	}
	if(!(nb_elem < MAX_PASSTHROUGH_DEV))
		return NULL;
	return lookup[nb_elem].hwid;
}

#else /* __KERNEL__ */

static inline int stat_hwid_file(void)
{
	char buf[_POSIX_PATH_MAX];
	struct stat st;

	snprintf(buf,_POSIX_PATH_MAX,"/dev/shm/qemu-kvm_%d",getpid());
	return stat(buf,&st);
}

static inline void* mmap_hwid_file(void)
{
	int rc,fd;
	void *ptr;
	char buf[_POSIX_PATH_MAX];

	snprintf(buf,_POSIX_PATH_MAX,"/qemu-kvm_%d",getpid());
	fd = shm_open(buf, O_RDWR, 0666);
	if(fd<0){
		rc = errno;
		goto err;
	}

	ptr = mmap(NULL,HWID_TABLE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ptr == MAP_FAILED){
		rc = errno;
		goto err1;
	}

	close(fd);
	return ptr;

err1:
	close(fd);
err:
	errno = rc;
	return NULL;
}

static inline int munmap_hwid_file(void *ptr)
{
	return munmap(ptr,HWID_TABLE_SIZE);
}

static inline void ack_hwid_request_number(void* ptr, int rc)
{
	struct hwid_req *r = &((struct hwid_table *)ptr)->req;
	r->rc=rc;
	r->lock++;
}

static inline int get_lookup(void *ptr, 
		int busnr, int devnr, int fcnnr)
{
	int nb_elem;
	struct vbdf_lookup *lookup = ((struct hwid_table *)ptr)->lookup;
		
	for(nb_elem=0; nb_elem < MAX_PASSTHROUGH_DEV; nb_elem++){
		if(!lookup[nb_elem].lock)
			continue;
		if( lookup[nb_elem].h_busnr == busnr &&
			lookup[nb_elem].h_devnr == devnr && 
			lookup[nb_elem].h_fcnnr == fcnnr)
				return nb_elem;
	}
	return -1;
}

static inline int add_vbdf_lookup(void *ptr,
	int gbusnr,	int gdevnr,	int gfcnnr,
	int busnr, int devnr, int fcnnr, char *hwid)
{
	int nb_elem;
	struct vbdf_lookup *lookup = ((struct hwid_table *)ptr)->lookup;

	for(nb_elem=0; nb_elem < MAX_PASSTHROUGH_DEV; nb_elem++){
		if(lookup[nb_elem].lock)
			continue;
		lookup[nb_elem].g_busnr = gbusnr;
		lookup[nb_elem].g_devnr = gdevnr;
		lookup[nb_elem].g_fcnnr = gfcnnr;
		lookup[nb_elem].h_busnr = busnr;
		lookup[nb_elem].h_devnr = devnr;
		lookup[nb_elem].h_fcnnr = fcnnr;
		strncpy(lookup[nb_elem].hwid,hwid,_POSIX_PATH_MAX);
		lookup[nb_elem].lock = 1;
		break;
	}
	if(!(nb_elem < MAX_PASSTHROUGH_DEV)){
		errno = ENOMEM;
		return -1;
	}
	return 0;
}
	
static inline int remove_vbdf_lookup(void *ptr,
	int gbusnr,	int gdevnr,	int gfcnnr,
	int busnr, int devnr, int fcnnr)
{
	int nb_elem;
	struct vbdf_lookup *lookup = ((struct hwid_table *)ptr)->lookup;
		
	for(nb_elem=0; nb_elem < MAX_PASSTHROUGH_DEV; nb_elem++){
		if(!lookup[nb_elem].lock)
			continue;
		if( lookup[nb_elem].g_busnr == gbusnr &&
			lookup[nb_elem].g_devnr == gdevnr && 
			lookup[nb_elem].g_fcnnr == gfcnnr &&
			lookup[nb_elem].h_busnr == busnr &&
			lookup[nb_elem].h_devnr == devnr && 
			lookup[nb_elem].h_fcnnr == fcnnr){

			lookup[nb_elem].lock = 0;
			lookup[nb_elem].g_busnr = 0;
			lookup[nb_elem].g_devnr = 0;
			lookup[nb_elem].g_fcnnr = 0;
			lookup[nb_elem].h_busnr = 0;
			lookup[nb_elem].h_devnr = 0;
			lookup[nb_elem].h_fcnnr = 0;
			memset(lookup[nb_elem].hwid,0,_POSIX_PATH_MAX);
			break;
		}
	}
	if(!(nb_elem < MAX_PASSTHROUGH_DEV)){
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

static inline int get_hwid(int busnr, int devnr, int fcnnr, char *hwid)
{
	int rc,fd;
	char buf[_POSIX_PATH_MAX];

	snprintf(buf,_POSIX_PATH_MAX,"/sys/bus/pci/devices/0000:%02x:%02x.%x/hwid",
		busnr,devnr,fcnnr);
	fd = open(buf, O_RDONLY, 0666);
	if(fd<0){
		rc = errno;
		goto err;
	}

	rc = read(fd,hwid,_POSIX_PATH_MAX);
	if(rc != _POSIX_PATH_MAX){
		rc = errno;
		goto err1;
	}

	close(fd);
	return 0;

err1:
	close(fd);
err:
	errno = rc;
	return -1;
}
#endif /*__KERNEL__*/

#endif

