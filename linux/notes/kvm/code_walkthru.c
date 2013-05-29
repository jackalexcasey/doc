
/*
 * A vcpu is taken out of VMX context
 */
static int vmx_handle_exit(struct kvm_vcpu *vcpu)
	trace_kvm_exit(exit_reason, kvm_rip_read(vcpu));
	...
	/* Vector the exit reason to appropriate handler */
	return kvm_vmx_exit_handlers[exit_reason](vcpu);

/*
 * In the handler there is things like ...
 */
static int (*kvm_vmx_exit_handlers[])(struct kvm_vcpu *vcpu) = {
	[EXIT_REASON_EXCEPTION_NMI]           = handle_exception,
	[EXIT_REASON_EXTERNAL_INTERRUPT]      = handle_external_interrupt,
	[EXIT_REASON_TRIPLE_FAULT]            = handle_triple_fault,
	[EXIT_REASON_NMI_WINDOW]	      = handle_nmi_window,
	[EXIT_REASON_IO_INSTRUCTION]          = handle_io,
	//...
	//...
};

/*
 * KVM Page fault case
 * handle_ept_violation and handle_exception translate into kvm_mmu_page_fault
 */
static int init_kvm_tdp_mmu(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu *context = &vcpu->arch.mmu;

	context->new_cr3 = nonpaging_new_cr3;
	context->page_fault = tdp_page_fault;

static int tdp_page_fault(struct kvm_vcpu *vcpu, gva_t gpa,
				u32 error_code)

int kvm_mmu_page_fault(struct kvm_vcpu *vcpu, gva_t cr2, u32 error_code)
{
	int r;
	enum emulation_result er;

	r = vcpu->arch.mmu.page_fault(vcpu, cr2, error_code);
	
	// If not resolve then go into emulate_instruction which can
	// kick back into user space
	er = emulate_instruction(vcpu, cr2, error_code, 0);

