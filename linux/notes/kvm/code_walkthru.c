
/*
 * A vcpu is taken out of VMX context
 */
static int vmx_handle_exit(struct kvm_vcpu *vcpu)
	trace_kvm_exit(exit_reason, kvm_rip_read(vcpu));
	...
	/* Vector the exit reason to appropriate handler */
	return kvm_vmx_exit_handlers[exit_reason](vcpu);

vmx_handle_exit
