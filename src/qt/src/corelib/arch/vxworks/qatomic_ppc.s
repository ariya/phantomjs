
	.align 2
	.globl q_atomic_test_and_set_int
	.globl .q_atomic_test_and_set_int
q_atomic_test_and_set_int:
.q_atomic_test_and_set_int:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_int-.q_atomic_test_and_set_int
	.short 25
	.byte "q_atomic_test_and_set_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_acquire_int
	.globl .q_atomic_test_and_set_acquire_int
q_atomic_test_and_set_acquire_int:
.q_atomic_test_and_set_acquire_int:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+16
	stwcx. 5,0,3
	bne-   $-16
        isync
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_acquire_int-.q_atomic_test_and_set_acquire_int
	.short 33
	.byte "q_atomic_test_and_set_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_release_int
	.globl .q_atomic_test_and_set_release_int
q_atomic_test_and_set_release_int:
.q_atomic_test_and_set_release_int:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_release_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_release_int-.q_atomic_test_and_set_release_int
	.short 33
	.byte "q_atomic_test_and_set_release_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_ptr
	.globl .q_atomic_test_and_set_ptr
q_atomic_test_and_set_ptr:
.q_atomic_test_and_set_ptr:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_ptr-.q_atomic_test_and_set_ptr
	.short 25
	.byte "q_atomic_test_and_set_ptr"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_acquire_ptr
	.globl .q_atomic_test_and_set_acquire_ptr
q_atomic_test_and_set_acquire_ptr:
.q_atomic_test_and_set_acquire_ptr:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+16
	stwcx. 5,0,3
	bne-   $-16
        isync
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_acquire_ptr-.q_atomic_test_and_set_acquire_ptr
	.short 25
	.byte "q_atomic_test_and_set_acquire_ptr"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_release_ptr
	.globl .q_atomic_test_and_set_release_ptr
q_atomic_test_and_set_release_ptr:
.q_atomic_test_and_set_release_ptr:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_release_ptr-.q_atomic_test_and_set_release_ptr
	.short 33
	.byte "q_atomic_test_and_set_release_ptr"
	.align 2

	.align 2
	.globl q_atomic_increment
	.globl .q_atomic_increment
q_atomic_increment:
.q_atomic_increment:
	lwarx  4,0,3
	addi   4,4,1
	stwcx. 4,0,3
	bne-   $-12
	mr     3,4
	blr
LT..q_atomic_increment:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_increment-.q_atomic_increment
	.short 18
	.byte "q_atomic_increment"
	.align 2

	.align 2
	.globl q_atomic_decrement
	.globl .q_atomic_decrement
q_atomic_decrement:
.q_atomic_decrement:
	lwarx  4,0,3
	subi   4,4,1
	stwcx. 4,0,3
	bne-   $-12
	mr     3,4
	blr
LT..q_atomic_decrement:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_decrement-.q_atomic_decrement
	.short 18
	.byte "q_atomic_decrement"
	.align 2

	.align 2
	.globl q_atomic_set_int
	.globl .q_atomic_set_int
q_atomic_set_int:
.q_atomic_set_int:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_set_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_set_int-.q_atomic_set_int
	.short 16
	.byte "q_atomic_set_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_acquire_int
	.globl .q_atomic_fetch_and_store_acquire_int
q_atomic_fetch_and_store_acquire_int:
.q_atomic_fetch_and_store_acquire_int:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_acquire_int-.q_atomic_fetch_and_store_acquire_int
	.short 16
	.byte "q_atomic_fetch_and_store_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_release_int
	.globl .q_atomic_fetch_and_store_release_int
q_atomic_fetch_and_store_release_int:
.q_atomic_fetch_and_store_release_int:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_release_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_release_int-.q_atomic_fetch_and_store_release_int
	.short 16
	.byte "q_atomic_fetch_and_store_release_int"
	.align 2

        .align 2
	.globl q_atomic_set_ptr
	.globl .q_atomic_set_ptr
q_atomic_set_ptr:
.q_atomic_set_ptr:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_set_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_set_ptr-.q_atomic_set_ptr
	.short 16
	.byte "q_atomic_set_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_acquire_ptr
	.globl .q_atomic_fetch_and_store_acquire_ptr
q_atomic_fetch_and_store_acquire_ptr:
.q_atomic_fetch_and_store_acquire_ptr:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_acquire_ptr-.q_atomic_fetch_and_store_acquire_ptr
	.short 16
	.byte "q_atomic_fetch_and_store_acquire_ptr"
	.align 2

        .align 2
	.globl q_atomic_fetch_and_store_release_ptr
	.globl .q_atomic_fetch_and_store_release_ptr
q_atomic_fetch_and_store_release_ptr:
.q_atomic_fetch_and_store_release_ptr:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_release_ptr-.q_atomic_fetch_and_store_release_ptr
	.short 16
	.byte "q_atomic_fetch_and_store_release_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_int
	.globl .q_atomic_fetch_and_add_int
q_atomic_fetch_and_add_int:
.q_atomic_fetch_and_add_int:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_int-.q_atomic_fetch_and_add_int
	.short 18
	.byte "q_atomic_fetch_and_add_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_acquire_int
	.globl .q_atomic_fetch_and_add_acquire_int
q_atomic_fetch_and_add_acquire_int:
.q_atomic_fetch_and_add_acquire_int:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_acquire_int-.q_atomic_fetch_and_add_acquire_int
	.short 18
	.byte "q_atomic_fetch_and_add_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_release_int
	.globl .q_atomic_fetch_and_add_release_int
q_atomic_fetch_and_add_release_int:
.q_atomic_fetch_and_add_release_int:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_release_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_release_int-.q_atomic_fetch_and_add_release_int
	.short 34
	.byte "q_atomic_fetch_and_add_release_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_ptr
	.globl .q_atomic_fetch_and_add_ptr
q_atomic_fetch_and_add_ptr:
.q_atomic_fetch_and_add_ptr:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_ptr-.q_atomic_fetch_and_add_ptr
	.short 26
	.byte "q_atomic_fetch_and_add_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_acquire_ptr
	.globl .q_atomic_fetch_and_add_acquire_ptr
q_atomic_fetch_and_add_acquire_ptr:
.q_atomic_fetch_and_add_acquire_ptr:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_acquire_ptr-.q_atomic_fetch_and_add_acquire_ptr
	.short 34
	.byte "q_atomic_fetch_and_add_acquire_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_release_ptr
	.globl .q_atomic_fetch_and_add_release_ptr
q_atomic_fetch_and_add_release_ptr:
.q_atomic_fetch_and_add_release_ptr:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_release_ptr-.q_atomic_fetch_and_add_release_ptr
	.short 34
	.byte "q_atomic_fetch_and_add_release_ptr"
	.align 2

_section_.text:
	.long _section_.text
