        .code64

	.globl q_atomic_increment
        .type q_atomic_increment,@function
        .section .text, "ax"
        .align 16
q_atomic_increment:
	lock
	incl (%rdi)
	setne %al
	ret
	.size q_atomic_increment,.-q_atomic_increment

	.globl q_atomic_decrement
        .type q_atomic_decrement,@function
        .section .text, "ax"
        .align 16
q_atomic_decrement:
	lock
	decl (%rdi)
	setne %al
	ret
	.size q_atomic_decrement,.-q_atomic_decrement

        .globl q_atomic_test_and_set_int
        .type q_atomic_test_and_set_int, @function
        .section .text, "ax"
        .align 16
q_atomic_test_and_set_int:
        movl %esi,%eax
        lock
        cmpxchgl %edx,(%rdi)
	movl $0,%eax
        sete %al
        ret
	.size q_atomic_test_and_set_int, . - q_atomic_test_and_set_int

	.globl q_atomic_set_int
        .type q_atomic_set_int,@function
        .section .text, "ax"
        .align 16
q_atomic_set_int:
        xchgl %esi,(%rdi)
	movl %esi,%eax
	ret
	.size q_atomic_set_int,.-q_atomic_set_int

        .globl q_atomic_fetch_and_add_int
        .type q_atomic_fetch_and_add_int,@function
        .section .text, "ax"
        .align 16
q_atomic_fetch_and_add_int:
        lock
        xaddl %esi,(%rdi)
        movl %esi, %eax
        ret
        .size q_atomic_fetch_and_add_int,.-q_atomic_fetch_and_add_int

        .globl q_atomic_test_and_set_ptr
        .type q_atomic_test_and_set_ptr, @function
        .section .text, "ax"
        .align 16
q_atomic_test_and_set_ptr:
        movq %rsi,%rax
        lock
        cmpxchgq %rdx,(%rdi)
	movq $0, %rax
        sete %al
        ret
        .size q_atomic_test_and_set_ptr, . - q_atomic_test_and_set_ptr

	.globl q_atomic_set_ptr
        .type q_atomic_set_ptr,@function
        .section .text, "ax"
        .align 16
q_atomic_set_ptr:
        xchgq %rsi,(%rdi)
	movq %rsi,%rax
	ret
	.size q_atomic_set_ptr,.-q_atomic_set_ptr

        .globl q_atomic_fetch_and_add_ptr
        .type q_atomic_fetch_and_add_ptr,@function
        .section .text, "ax"
        .align 16
q_atomic_fetch_and_add_ptr:
        lock
        xaddq %rsi,(%rdi)
        movq %rsi,%rax
        ret
        .size q_atomic_fetch_and_add_ptr,.-q_atomic_fetch_and_add_ptr
