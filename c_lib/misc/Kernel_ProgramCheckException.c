In traps.c

                           Table 5-17. Program Interrupt Exception Conditions
       Exception                                                           Cause
Illegal instruction    An illegal instruction exception always occurs when execution of any of the following kinds of instructions
exception              is attempted.
                        • A reserved-illegal instruction
                        • In user mode, an mtspr or mfspr that specifies an SPRN value with SPRN[5] = 0 (user-mode
                          accessible) that represents an unimplemented SPR
                        • (EIS) If an invalid SPR address is accessible only in supervisor mode and the processor is in
                          supervisor mode (MSR[PR] = 0), results are undefined.
                        • (EIS) If the invalid SPR address is accessible only in the supervisor mode and the processor is in user
                          mode (MSR[PR] = 1), a privileged instruction exception is taken.
                       An illegal instruction exception may occur when execution is attempted of any of the following kinds of
                       instructions. If the exception does not occur, the alternative is shown in parentheses.
                        • An instruction that is in invalid form (boundedly undefined results). On the e500, all instructions have
                          invalid forms cause boundedly undefined results.
                        • A reserved no-op instruction (no-operation performed is preferred). There are no reserved no-ops for
                          the e500.
                        • A defined or allocated instruction that is not implemented (unimplemented operation exception).
                          Unimplemented Book E instructions such as mfapidi, mfdcr, and mtdcr take an illegal instruction
                          exception.
                        • The EIS defines that an attempt to execute a 64-bit Book E instruction causes an illegal instruction
                          exception.
Privileged instruction Occurs when MSR[PR] = 1 and execution is attempted of any of the following:
exception               • A privileged instruction
                        • An mtspr or mfspr instruction that specifies a privileged SPR (SPRN[5] = 1)
                        • (EIS) An mtpmr or mfpmr instruction that specifies a privileged PMR (PMRN[5] = 1)
Trap exception         A trap exception occurs when any of the conditions specified in a trap instruction are met.
Unimplemented          An unimplemented operation exception may occur when a defined or allocated instruction is encountered
operation exception    that is not implemented. Otherwise an illegal instruction exception occurs. On the e500, these instructions
                       are mfapidi, mfdcr, and mtdcr and they take an illegal instruction exception.


The CPU generates program_check_exception

{
	if (reason & REASON_TRAP) {
		/* trap exception */
		if (debugger_bpt(regs))
			return;
		if (check_bug_trap(regs)) {
			regs->nip += 4;
			return;
		}
		/*If return is 0 raise exception. Can be kernel or User mode*/
		_exception(SIGTRAP, regs, TRAP_BRKPT, 0);


void _exception(int signr, struct pt_regs *regs, int code, unsigned long addr)
{
	siginfo_t info;

	if (!user_mode(regs)) {
		debugger(regs);
		die("Exception in kernel mode", regs, signr);
	}

}

/*
 * Define an illegal instr to trap on the bug.
 * We don't use 0 because that marks the end of a function
 * in the ELF ABI.  That's "Boo Boo" in case you wonder...
 */
#define BUG_WARNING_TRAP	0x1000000
#define BUG_OPCODE .long 0x00b00b00  /* For asm */
#define BUG_ILLEGAL_INSTR "0x00b00b00" /* For BUG macro */

This is using a special txt section to put an entry for BUG report in a table 

#define BUG_ON(x) do {						\
	if (__builtin_constant_p(x)) {				\
		if (x)						\
			BUG();					\
	} else {						\
		__asm__ __volatile__(				\
		"1:	"PPC_TLNEI"	%0,0\n"			\
		".section __bug_table,\"a\"\n"			\
		"\t"PPC_LONG"	1b,%1,%2,%3\n"			\
		".previous"					\
		: : "r" ((long)(x)), "i" (__LINE__),		\
		    "i" (__FILE__), "i" (__FUNCTION__));	\
	}							\
} while (0)

struct bug_entry *find_bug(unsigned long bugaddr)
{
	struct bug_entry *bug;

	for (bug = __start___bug_table; bug < __stop___bug_table; ++bug)
		if (bugaddr == bug->bug_addr)
			return bug;
	return module_find_bug(bugaddr);
}

int check_bug_trap(struct pt_regs *regs)
{
	if (regs->msr & MSR_PR)
		return 0;	/* not in kernel */
	addr = regs->nip;	/* address of trap instruction */
	if (addr < PAGE_OFFSET)
		return 0;
	bug = find_bug(regs->nip);	/*Look in the BUG table*/
	if (bug == NULL)
		return 0;

	/*Exception belong to kernel:*/
	if (bug->line & BUG_WARNING_TRAP) {
		/* this is a WARN_ON rather than BUG/BUG_ON */
		printk(KERN_ERR "Badness in %s at %s:%ld\n",
		       bug->function, bug->file,
		       bug->line & ~BUG_WARNING_TRAP);
		dump_stack();
		return 1;
	}

	/*This is the real BUG macro*/
	printk(KERN_CRIT "kernel BUG in %s at %s:%ld!\n",
	       bug->function, bug->file, bug->line);

	return 0;

}


SO the Macro BUG/BUG_ON are putting an invalid opcaode if condition is met.
The kernel check_bug_trap is making/reporting the condition

