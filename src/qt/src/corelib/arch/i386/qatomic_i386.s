	.text

	.align 4,0x90
	.globl q_atomic_test_and_set_int
q_atomic_test_and_set_int:
	movl		 4(%esp),%ecx
	movl		 8(%esp),%eax
	movl		12(%esp),%edx
	lock
	cmpxchgl	%edx,(%ecx)
	mov		$0,%eax
 	sete		%al
	ret
	.align 4,0x90
	.type q_atomic_test_and_set_int,@function
	.size q_atomic_test_and_set_int,.-q_atomic_test_and_set_int

	.align 4,0x90
	.globl q_atomic_test_and_set_ptr
q_atomic_test_and_set_ptr:
	movl		 4(%esp),%ecx
	movl		 8(%esp),%eax
	movl		12(%esp),%edx
	lock 
	cmpxchgl	%edx,(%ecx)
	mov		$0,%eax
	sete		%al
	ret
	.align    4,0x90
	.type	q_atomic_test_and_set_ptr,@function
	.size	q_atomic_test_and_set_ptr,.-q_atomic_test_and_set_ptr

	.align 4,0x90
	.globl q_atomic_increment
q_atomic_increment:
	movl 4(%esp), %ecx
	lock 
	incl (%ecx)
	mov $0,%eax
	setne %al
	ret
	.align 4,0x90
	.type q_atomic_increment,@function
	.size	q_atomic_increment,.-q_atomic_increment

	.align 4,0x90
	.globl q_atomic_decrement
q_atomic_decrement:
	movl 4(%esp), %ecx
	lock 
	decl (%ecx)
	mov $0,%eax
	setne %al
	ret
	.align 4,0x90
	.type q_atomic_decrement,@function
	.size	q_atomic_decrement,.-q_atomic_decrement

	.align 4,0x90
	.globl q_atomic_set_int
q_atomic_set_int:
	mov 4(%esp),%ecx
	mov 8(%esp),%eax
	xchgl %eax,(%ecx)
	ret	
	.align 4,0x90
	.type q_atomic_set_int,@function
	.size	q_atomic_set_int,.-q_atomic_set_int

	.align 4,0x90
	.globl q_atomic_set_ptr
q_atomic_set_ptr:
	mov 4(%esp),%ecx
	mov 8(%esp),%eax
	xchgl %eax,(%ecx)
	ret	
	.align 4,0x90
	.type q_atomic_set_ptr,@function
	.size	q_atomic_set_ptr,.-q_atomic_set_ptr

        .align 4,0x90
        .globl q_atomic_fetch_and_add_int
q_atomic_fetch_and_add_int:
        mov 4(%esp),%ecx
        mov 8(%esp),%eax
        lock
        xadd %eax,(%ecx)
        ret
        .align 4,0x90
        .type q_atomic_fetch_and_add_int,@function
        .size q_atomic_fetch_and_add_int,.-q_atomic_fetch_and_add_int

        .align 4,0x90
        .globl q_atomic_fetch_and_add_ptr
q_atomic_fetch_and_add_ptr:
        mov 4(%esp),%ecx
        mov 8(%esp),%eax
        lock
        xadd %eax,(%ecx)
        ret
        .align 4,0x90
        .type q_atomic_fetch_and_add_ptr,@function
        .size q_atomic_fetch_and_add_ptr,.-q_atomic_fetch_and_add_ptr
