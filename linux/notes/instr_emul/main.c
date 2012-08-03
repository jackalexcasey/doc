#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
enum reason_type {
	NOT_ME,	/* page fault is not in regions */
	NOTHING,	/* access others point in regions */
	REG_READ,	/* read from addr to reg */
	REG_WRITE,	/* write from reg to addr */
	IMM_WRITE,	/* write from imm to addr */
	OTHERS	/* Other instructions can not intercept */
};

static unsigned char prefix_codes[] = {
	0x66, 0x67, 0x2E, 0x3E, 0x26, 0x64, 0x65, 0x36,
	0xF0, 0xF3, 0xF2,
	/* REX Prefixes */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
};
/* AMD64 Manual 3, Appendix A*/
static unsigned int reg_rop[] = {
	0x8A, 0x8B, 0xB60F, 0xB70F, 0xBE0F, 0xBF0F
};
static unsigned int reg_wop[] = { 0x88, 0x89 };
static unsigned int imm_wop[] = { 0xC6, 0xC7 };
static unsigned int rw8[] = { 0xC6, 0x88, 0x8A };
static unsigned int rw32[] = {
	0xC7, 0x89, 0x8B, 0xB60F, 0xB70F, 0xBE0F, 0xBF0F
};
/* 8 bit only */
static unsigned int mw8[] = { 0xC6, 0x88, 0x8A, 0xB60F, 0xBE0F };
/* 16 bit only */
static unsigned int mw16[] = { 0xB70F, 0xBF0F };
/* 16 or 32 bit */
static unsigned int mw32[] = { 0xC7 };
/* 16, 32 or 64 bit */
static unsigned int mw64[] = { 0x89, 0x8B };

struct prefix_bits {
	unsigned shorted:1;
	unsigned enlarged:1;
	unsigned rexr:1;
	unsigned rex:1;
};

static int skip_prefix(unsigned char *addr, struct prefix_bits *prf)
{
	int i;
	unsigned char *p = addr;
	prf->shorted = 0;
	prf->enlarged = 0;
	prf->rexr = 0;
	prf->rex = 0;

restart:
	for (i = 0; i < ARRAY_SIZE(prefix_codes); i++) {
		if (*p == prefix_codes[i]) {
			if (*p == 0x66)
				prf->shorted = 1;
#ifdef __amd64__
			if ((*p & 0xf8) == 0x48)
				prf->enlarged = 1;
			if ((*p & 0xf4) == 0x44)
				prf->rexr = 1;
			if ((*p & 0xf0) == 0x40)
				prf->rex = 1;
#endif
			p++;
			goto restart;
		}
	}

	return (p - addr);
}

static int get_opcode(unsigned char *addr, unsigned int *opcode)
{
	int len;

	if (*addr == 0x0F) {
		/* 0x0F is extension instruction */
		*opcode = *(unsigned short *)addr;
		len = 2;
	} else {
		*opcode = *addr;
		len = 1;
	}

	return len;
}

#define CHECK_OP_TYPE(opcode, array, type) \
	for (i = 0; i < ARRAY_SIZE(array); i++) { \
		if (array[i] == opcode) { \
			rv = type; \
			goto exit; \
		} \
	}

enum reason_type get_ins_type(unsigned long ins_addr)
{
	unsigned int opcode;
	unsigned char *p;
	struct prefix_bits prf;
	int i;
	enum reason_type rv = OTHERS;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);

	CHECK_OP_TYPE(opcode, reg_rop, REG_READ);
	CHECK_OP_TYPE(opcode, reg_wop, REG_WRITE);
	CHECK_OP_TYPE(opcode, imm_wop, IMM_WRITE);

exit:
	return rv;
}
#undef CHECK_OP_TYPE

#if 0
static unsigned int get_ins_reg_width(unsigned long ins_addr)
{
	unsigned int opcode;
	unsigned char *p;
	struct prefix_bits prf;
	int i;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);

	for (i = 0; i < ARRAY_SIZE(rw8); i++)
		if (rw8[i] == opcode)
			return 1;

	for (i = 0; i < ARRAY_SIZE(rw32); i++)
		if (rw32[i] == opcode)
			return prf.shorted ? 2 : (prf.enlarged ? 8 : 4);

	fprintf(stderr, "mmiotrace: Unknown opcode 0x%02x\n", opcode);
	return 0;
}
#endif

unsigned int get_ins_mem_width(unsigned long ins_addr)
{
	unsigned int opcode;
	unsigned char *p;
	struct prefix_bits prf;
	int i;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);

	for (i = 0; i < ARRAY_SIZE(mw8); i++)
		if (mw8[i] == opcode)
			return 1;

	for (i = 0; i < ARRAY_SIZE(mw16); i++)
		if (mw16[i] == opcode)
			return 2;

	for (i = 0; i < ARRAY_SIZE(mw32); i++)
		if (mw32[i] == opcode)
			return prf.shorted ? 2 : 4;

	for (i = 0; i < ARRAY_SIZE(mw64); i++)
		if (mw64[i] == opcode)
			return prf.shorted ? 2 : (prf.enlarged ? 8 : 4);

	fprintf(stderr, "mmiotrace: Unknown opcode 0x%02x\n", opcode);
	return 0;
}

unsigned long get_ins_length(unsigned long ins_addr, void *regs)
{
	unsigned int opcode;
	unsigned char *p;
	struct prefix_bits prf;
	unsigned char mod_rm;
	int i, rv, reg;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);

	for (i = 0; i < ARRAY_SIZE(reg_rop); i++)
		if (reg_rop[i] == opcode) {
			rv = REG_READ;
			goto do_work;
		}

	for (i = 0; i < ARRAY_SIZE(reg_wop); i++)
		if (reg_wop[i] == opcode) {
			rv = REG_WRITE;
			goto do_work;
		}

	fprintf(stderr, "mmiotrace: Not an immediate instruction, opcode "
							"0x%02x\n", opcode);
	return 0;

do_work:
	mod_rm = *p;
	reg = ((mod_rm >> 3) & 0x7) | (prf.rexr << 3);

	if(rv == REG_WRITE){
		if(*p == 0x50)
			p++;
	}
	if(rv == REG_READ){
		if(*p == 0x10){
			p++;
			return p - (unsigned char *)ins_addr;
		}
		if(*p == 0x0){
			p++;
			return p - (unsigned char *)ins_addr;
		}
	}

	/* Skip register */
	p++;

	if(rv == REG_WRITE){
		if(prf.enlarged){
			for (i = 0; i < ARRAY_SIZE(mw64); i++){
				if (mw64[i] == opcode){
					p++;
				}
			}
		}

		return p - (unsigned char *)ins_addr;
	}

	if(!prf.enlarged)
		p++;
	return p - (unsigned char *)ins_addr;
}

#if 0
		if(type == REG_READ){
			switch(my_trace->width){
				case 1:
					regs->ip += length+2;
					//b07:       41 0f b6 14 24          movzbl (%r12),%edx
					//	PREFIX 41 0f
					//	OPCODE b6
					break;
				case 2:
					regs->ip += length+2;
					//aa1:       41 0f b7 14 24          movzwl (%r12),%edx
					break;
				case 4:
					regs->ip += length+2;
					//ac6:       41 8b 14 24             mov    (%r12),%edx
					break;
				case 8:
					regs->ip += length+1;
					// aea:       48 8b 13                mov    (%rbx),%rdx
					break;
				default:
					BUG();
			}
		}
		else if(type == REG_WRITE){
			switch(my_trace->width){
				case 1:
					regs->ip += length+1;
					//8f0:       88 02                   mov    %al,(%rdx)
					//OPCODE 2 Width 1 Instr length 1
					break;
				case 2:
					regs->ip += length+1;
					//950:       66 89 02                mov    %ax,(%rdx)
					//OPCODE 2 Width 2 Instr length 2
					//
					break;
				case 4:
					regs->ip += length+1;
					//9b0:       89 02                   mov    %eax,(%rdx)
					//OPCODE 2 Width 4 Instr length 1
					//ffffffff8128ae6c:       89 10                   mov    %edx,(%rax)
					//fffffff8128ae71:       89 50 04                mov    %edx,0x4(%rax)
					//
					break;
				case 8:
					regs->ip += length+1 + 1;
					//a19:       49 89 4c cd 00          mov    %rcx,0x0(%r13,%rcx,8)
					//a25:       48 89 04 32             mov    %rax,(%rdx,%rsi,1)
					//
					//OPCODE 2 Width 8 Instr length 2
					break;
				default:
					BUG();
			}
		}
#endif

//b07:       41 0f b6 14 24          movzbl (%r12),%edx
unsigned char instr0[] = {0x41,0x0f,0xb6,0x14,0x24,0x77,0x88,0x99};

//aa1:       41 0f b7 14 24          movzwl (%r12),%edx
unsigned char instr1[] = {0x41,0x0f,0xb7,0x14,0x24,0x77,0x88,0x99};

//ac6:       41 8b 14 24             mov    (%r12),%edx
unsigned char instr2[] = {0x41,0x8b,0x14,0x24,0x77,0x88,0x99};

// aea:       48 8b 13                mov    (%rbx),%rdx
unsigned char instr3[] = {0x48,0x8b,0x13,0x77,0x88,0x99};



//8f0:       88 02                   mov    %al,(%rdx)
unsigned char instr4[] = {0x88,0x02,0x77,0x88,0x99};

//950:       66 89 02                mov    %ax,(%rdx)
unsigned char instr5[] = {0x66,0x89,0x02,0x77,0x88,0x99};

//9b0:       89 02                   mov    %eax,(%rdx)
unsigned char instr6[] = {0x89,0x02,0x77,0x88,0x99};

//a19:       49 89 4c cd 00          mov    %rcx,0x0(%r13,%rcx,8)
unsigned char instr7[] = {0x49,0x89,0x4c,0xcd,0x00,0x77,0x88,0x99};

//a25:       48 89 04 32             mov    %rax,(%rdx,%rsi,1)
unsigned char instr8[] = {0x48,0x89,0x04,0x32,0x77,0x88,0x99};

//ffffffff8128af11:       89 50 04                mov    %edx,0x4(%rax)
unsigned char instr9[] = {0x89,0x50,0x04,0x77,0x88,0x99};

//ffffffff8128b871:       8b 10                   mov    (%rax),%edx
unsigned char instr10[] = {0x8b,0x10,0x77,0x88,0x99};

//  5b9ab8:       8b 00                   mov    (%rax),%eax
unsigned char instr11[] = {0x8b,0x00,0x77,0x88,0x99};

//  OPCODE 1 Width 4 Instr length 3

int main(void)
{
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr0, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr1, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr2, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr3, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr4, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr5, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr6, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr7, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr8, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr9, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr10, NULL));
	printf("Lenght %d\n",(int)get_ins_length((unsigned long)instr11, NULL));
	return 0;

}

#if 0
/*
 * Define register ident in mod/rm byte.
 * Note: these are NOT the same as in ptrace-abi.h.
 */
enum {
	arg_AL = 0,
	arg_CL = 1,
	arg_DL = 2,
	arg_BL = 3,
	arg_AH = 4,
	arg_CH = 5,
	arg_DH = 6,
	arg_BH = 7,

	arg_AX = 0,
	arg_CX = 1,
	arg_DX = 2,
	arg_BX = 3,
	arg_SP = 4,
	arg_BP = 5,
	arg_SI = 6,
	arg_DI = 7,
#ifdef __amd64__
	arg_R8  = 8,
	arg_R9  = 9,
	arg_R10 = 10,
	arg_R11 = 11,
	arg_R12 = 12,
	arg_R13 = 13,
	arg_R14 = 14,
	arg_R15 = 15
#endif
};

static unsigned char *get_reg_w8(int no, int rex, struct pt_regs *regs)
{
	unsigned char *rv = NULL;

	switch (no) {
	case arg_AL:
		rv = (unsigned char *)&regs->ax;
		break;
	case arg_BL:
		rv = (unsigned char *)&regs->bx;
		break;
	case arg_CL:
		rv = (unsigned char *)&regs->cx;
		break;
	case arg_DL:
		rv = (unsigned char *)&regs->dx;
		break;
#ifdef __amd64__
	case arg_R8:
		rv = (unsigned char *)&regs->r8;
		break;
	case arg_R9:
		rv = (unsigned char *)&regs->r9;
		break;
	case arg_R10:
		rv = (unsigned char *)&regs->r10;
		break;
	case arg_R11:
		rv = (unsigned char *)&regs->r11;
		break;
	case arg_R12:
		rv = (unsigned char *)&regs->r12;
		break;
	case arg_R13:
		rv = (unsigned char *)&regs->r13;
		break;
	case arg_R14:
		rv = (unsigned char *)&regs->r14;
		break;
	case arg_R15:
		rv = (unsigned char *)&regs->r15;
		break;
#endif
	default:
		break;
	}

	if (rv)
		return rv;

	if (rex) {
		/*
		 * If REX prefix exists, access low bytes of SI etc.
		 * instead of AH etc.
		 */
		switch (no) {
		case arg_SI:
			rv = (unsigned char *)&regs->si;
			break;
		case arg_DI:
			rv = (unsigned char *)&regs->di;
			break;
		case arg_BP:
			rv = (unsigned char *)&regs->bp;
			break;
		case arg_SP:
			rv = (unsigned char *)&regs->sp;
			break;
		default:
			break;
		}
	} else {
		switch (no) {
		case arg_AH:
			rv = 1 + (unsigned char *)&regs->ax;
			break;
		case arg_BH:
			rv = 1 + (unsigned char *)&regs->bx;
			break;
		case arg_CH:
			rv = 1 + (unsigned char *)&regs->cx;
			break;
		case arg_DH:
			rv = 1 + (unsigned char *)&regs->dx;
			break;
		default:
			break;
		}
	}

	if (!rv)
		fprintf(stderr, "mmiotrace: Error reg no# %d\n", no);

	return rv;
}

static unsigned long *get_reg_w32(int no, struct pt_regs *regs)
{
	unsigned long *rv = NULL;

	switch (no) {
	case arg_AX:
		rv = &regs->ax;
		break;
	case arg_BX:
		rv = &regs->bx;
		break;
	case arg_CX:
		rv = &regs->cx;
		break;
	case arg_DX:
		rv = &regs->dx;
		break;
	case arg_SP:
		rv = &regs->sp;
		break;
	case arg_BP:
		rv = &regs->bp;
		break;
	case arg_SI:
		rv = &regs->si;
		break;
	case arg_DI:
		rv = &regs->di;
		break;
#ifdef __amd64__
	case arg_R8:
		rv = &regs->r8;
		break;
	case arg_R9:
		rv = &regs->r9;
		break;
	case arg_R10:
		rv = &regs->r10;
		break;
	case arg_R11:
		rv = &regs->r11;
		break;
	case arg_R12:
		rv = &regs->r12;
		break;
	case arg_R13:
		rv = &regs->r13;
		break;
	case arg_R14:
		rv = &regs->r14;
		break;
	case arg_R15:
		rv = &regs->r15;
		break;
#endif
	default:
		fprintf(stderr, "mmiotrace: Error reg no# %d\n", no);
	}

	return rv;
}

static unsigned long getset_ins_reg_val(unsigned long ins_addr, 
	struct pt_regs *regs, unsigned long value, int ops)
{
	unsigned int opcode;
	unsigned char mod_rm;
	int reg;
	unsigned char *p;
	struct prefix_bits prf;
	int i;
	unsigned long rv;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);
	for (i = 0; i < ARRAY_SIZE(reg_rop); i++)
		if (reg_rop[i] == opcode) {
			rv = REG_READ;
			goto do_work;
		}

	for (i = 0; i < ARRAY_SIZE(reg_wop); i++)
		if (reg_wop[i] == opcode) {
			rv = REG_WRITE;
			goto do_work;
		}

	fprintf(stderr, "mmiotrace: Not a register instruction, opcode "
							"0x%02x\n", opcode);
	goto err;

do_work:
	mod_rm = *p;
	reg = ((mod_rm >> 3) & 0x7) | (prf.rexr << 3);
	switch (get_ins_reg_width(ins_addr)) {
	case 1:
		if(!ops)
			return *get_reg_w8(reg, prf.rex, regs);
		*get_reg_w8(reg, prf.rex, regs) = (unsigned char)value;
		return 0;

	case 2:
		if(!ops)
			return *(unsigned short *)get_reg_w32(reg, regs);
		*(unsigned short *)get_reg_w32(reg, regs) = (unsigned short)value;
		return 0;

	case 4:
		if(!ops)
			return *(unsigned int *)get_reg_w32(reg, regs);
		*(unsigned int *)get_reg_w32(reg, regs) = (unsigned int)value;
		return 0;

#ifdef __amd64__
	case 8:
		if(!ops)
			return *(unsigned long *)get_reg_w32(reg, regs);
		*(unsigned long *)get_reg_w32(reg, regs) = (unsigned long) value;
		return 0;
#endif

	default:
		fprintf(stderr, "mmiotrace: Error width# %d\n", reg);
	}

err:
	return 0;
}

unsigned long get_ins_reg_val(unsigned long ins_addr, struct pt_regs *regs)
{
	return getset_ins_reg_val(ins_addr,regs,0,0);
}

void set_ins_reg_val(unsigned long ins_addr, struct pt_regs *regs, unsigned long value)
{
	getset_ins_reg_val(ins_addr,regs,value,1);
}


unsigned long get_ins_imm_val(unsigned long ins_addr)
{
	unsigned int opcode;
	unsigned char mod_rm;
	unsigned char mod;
	unsigned char *p;
	struct prefix_bits prf;
	int i;
	unsigned long rv;

	p = (unsigned char *)ins_addr;
	p += skip_prefix(p, &prf);
	p += get_opcode(p, &opcode);
	for (i = 0; i < ARRAY_SIZE(imm_wop); i++)
		if (imm_wop[i] == opcode) {
			rv = IMM_WRITE;
			goto do_work;
		}

	fprintf(stderr, "mmiotrace: Not an immediate instruction, opcode "
							"0x%02x\n", opcode);
	goto err;

do_work:
	mod_rm = *p;
	mod = mod_rm >> 6;
	p++;
	switch (mod) {
	case 0:
		/* if r/m is 5 we have a 32 disp (IA32 Manual 3, Table 2-2)  */
		/* AMD64: XXX Check for address size prefix? */
		if ((mod_rm & 0x7) == 0x5)
			p += 4;
		break;

	case 1:
		p += 1;
		break;

	case 2:
		p += 4;
		break;

	case 3:
	default:
		fprintf(stderr, "mmiotrace: not a memory access instruction "
						"at 0x%lx, rm_mod=0x%02x\n",
						ins_addr, mod_rm);
	}

	switch (get_ins_reg_width(ins_addr)) {
	case 1:
		return *(unsigned char *)p;

	case 2:
		return *(unsigned short *)p;

	case 4:
		return *(unsigned int *)p;

#ifdef __amd64__
	case 8:
		return *(unsigned long *)p;
#endif

	default:
		fprintf(stderr, "mmiotrace: Error: width.\n");
	}

err:
	return 0;
}
#endif




