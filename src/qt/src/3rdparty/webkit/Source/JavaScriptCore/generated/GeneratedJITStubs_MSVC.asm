    AREA Trampoline, CODE

    EXPORT ctiTrampoline
    EXPORT ctiVMThrowTrampoline
    EXPORT ctiOpThrowNotCaught

ctiTrampoline PROC
    stmdb sp!, {r1-r3}
    stmdb sp!, {r4-r8, lr}
    sub sp, sp, #68 ; sync with PRESERVEDR4_OFFSET
    mov r4, r2
    mov r5, #512
    ; r0 contains the code
    mov lr, pc
    bx r0
    add sp, sp, #68 ; sync with PRESERVEDR4_OFFSET
    ldmia sp!, {r4-r8, lr}
    add sp, sp, #12
    bx lr
ctiTrampoline ENDP

ctiVMThrowTrampoline PROC
    mov r0, sp
    mov lr, pc
    bl cti_vm_throw
ctiOpThrowNotCaught
    add sp, sp, #68 ; sync with PRESERVEDR4_OFFSET
    ldmia sp!, {r4-r8, lr}
    add sp, sp, #12
    bx lr
ctiVMThrowTrampoline ENDP

    EXPORT cti_op_create_this
    IMPORT JITStubThunked_op_create_this
cti_op_create_this PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_create_this
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_create_this ENDP

    EXPORT cti_op_convert_this
    IMPORT JITStubThunked_op_convert_this
cti_op_convert_this PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_convert_this
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_convert_this ENDP

    EXPORT cti_op_convert_this_strict
    IMPORT JITStubThunked_op_convert_this_strict
cti_op_convert_this_strict PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_convert_this_strict
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_convert_this_strict ENDP

    EXPORT cti_op_add
    IMPORT JITStubThunked_op_add
cti_op_add PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_add
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_add ENDP

    EXPORT cti_op_pre_inc
    IMPORT JITStubThunked_op_pre_inc
cti_op_pre_inc PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_pre_inc
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_pre_inc ENDP

    EXPORT cti_timeout_check
    IMPORT JITStubThunked_timeout_check
cti_timeout_check PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_timeout_check
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_timeout_check ENDP

    EXPORT cti_register_file_check
    IMPORT JITStubThunked_register_file_check
cti_register_file_check PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_register_file_check
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_register_file_check ENDP

    EXPORT cti_op_loop_if_lesseq
    IMPORT JITStubThunked_op_loop_if_lesseq
cti_op_loop_if_lesseq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_loop_if_lesseq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_loop_if_lesseq ENDP

    EXPORT cti_op_new_object
    IMPORT JITStubThunked_op_new_object
cti_op_new_object PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_new_object
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_new_object ENDP

    EXPORT cti_op_put_by_id_generic
    IMPORT JITStubThunked_op_put_by_id_generic
cti_op_put_by_id_generic PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_generic
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_generic ENDP

    EXPORT cti_op_put_by_id_direct_generic
    IMPORT JITStubThunked_op_put_by_id_direct_generic
cti_op_put_by_id_direct_generic PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_direct_generic
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_direct_generic ENDP

    EXPORT cti_op_get_by_id_generic
    IMPORT JITStubThunked_op_get_by_id_generic
cti_op_get_by_id_generic PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_generic
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_generic ENDP

    EXPORT cti_op_put_by_id
    IMPORT JITStubThunked_op_put_by_id
cti_op_put_by_id PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id ENDP

    EXPORT cti_op_put_by_id_direct
    IMPORT JITStubThunked_op_put_by_id_direct
cti_op_put_by_id_direct PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_direct
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_direct ENDP

    EXPORT cti_op_put_by_id_fail
    IMPORT JITStubThunked_op_put_by_id_fail
cti_op_put_by_id_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_fail ENDP

    EXPORT cti_op_put_by_id_direct_fail
    IMPORT JITStubThunked_op_put_by_id_direct_fail
cti_op_put_by_id_direct_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_direct_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_direct_fail ENDP

    EXPORT cti_op_put_by_id_transition_realloc
    IMPORT JITStubThunked_op_put_by_id_transition_realloc
cti_op_put_by_id_transition_realloc PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_id_transition_realloc
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_id_transition_realloc ENDP

    EXPORT cti_op_get_by_id_method_check
    IMPORT JITStubThunked_op_get_by_id_method_check
cti_op_get_by_id_method_check PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_method_check
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_method_check ENDP

    EXPORT cti_op_get_by_id
    IMPORT JITStubThunked_op_get_by_id
cti_op_get_by_id PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id ENDP

    EXPORT cti_op_get_by_id_self_fail
    IMPORT JITStubThunked_op_get_by_id_self_fail
cti_op_get_by_id_self_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_self_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_self_fail ENDP

    EXPORT cti_op_get_by_id_getter_stub
    IMPORT JITStubThunked_op_get_by_id_getter_stub
cti_op_get_by_id_getter_stub PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_getter_stub
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_getter_stub ENDP

    EXPORT cti_op_get_by_id_custom_stub
    IMPORT JITStubThunked_op_get_by_id_custom_stub
cti_op_get_by_id_custom_stub PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_custom_stub
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_custom_stub ENDP

    EXPORT cti_op_get_by_id_proto_list
    IMPORT JITStubThunked_op_get_by_id_proto_list
cti_op_get_by_id_proto_list PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_proto_list
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_proto_list ENDP

    EXPORT cti_op_get_by_id_proto_list_full
    IMPORT JITStubThunked_op_get_by_id_proto_list_full
cti_op_get_by_id_proto_list_full PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_proto_list_full
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_proto_list_full ENDP

    EXPORT cti_op_get_by_id_proto_fail
    IMPORT JITStubThunked_op_get_by_id_proto_fail
cti_op_get_by_id_proto_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_proto_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_proto_fail ENDP

    EXPORT cti_op_get_by_id_array_fail
    IMPORT JITStubThunked_op_get_by_id_array_fail
cti_op_get_by_id_array_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_array_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_array_fail ENDP

    EXPORT cti_op_get_by_id_string_fail
    IMPORT JITStubThunked_op_get_by_id_string_fail
cti_op_get_by_id_string_fail PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_id_string_fail
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_id_string_fail ENDP

    EXPORT cti_op_check_has_instance
    IMPORT JITStubThunked_op_check_has_instance
cti_op_check_has_instance PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_check_has_instance
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_check_has_instance ENDP

    EXPORT cti_op_instanceof
    IMPORT JITStubThunked_op_instanceof
cti_op_instanceof PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_instanceof
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_instanceof ENDP

    EXPORT cti_op_del_by_id
    IMPORT JITStubThunked_op_del_by_id
cti_op_del_by_id PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_del_by_id
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_del_by_id ENDP

    EXPORT cti_op_mul
    IMPORT JITStubThunked_op_mul
cti_op_mul PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_mul
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_mul ENDP

    EXPORT cti_op_new_func
    IMPORT JITStubThunked_op_new_func
cti_op_new_func PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_new_func
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_new_func ENDP

    EXPORT cti_op_call_jitCompile
    IMPORT JITStubThunked_op_call_jitCompile
cti_op_call_jitCompile PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_call_jitCompile
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_call_jitCompile ENDP

    EXPORT cti_op_construct_jitCompile
    IMPORT JITStubThunked_op_construct_jitCompile
cti_op_construct_jitCompile PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_construct_jitCompile
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_construct_jitCompile ENDP

    EXPORT cti_op_call_arityCheck
    IMPORT JITStubThunked_op_call_arityCheck
cti_op_call_arityCheck PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_call_arityCheck
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_call_arityCheck ENDP

    EXPORT cti_op_construct_arityCheck
    IMPORT JITStubThunked_op_construct_arityCheck
cti_op_construct_arityCheck PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_construct_arityCheck
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_construct_arityCheck ENDP

    EXPORT cti_vm_lazyLinkCall
    IMPORT JITStubThunked_vm_lazyLinkCall
cti_vm_lazyLinkCall PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_vm_lazyLinkCall
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_vm_lazyLinkCall ENDP

    EXPORT cti_vm_lazyLinkConstruct
    IMPORT JITStubThunked_vm_lazyLinkConstruct
cti_vm_lazyLinkConstruct PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_vm_lazyLinkConstruct
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_vm_lazyLinkConstruct ENDP

    EXPORT cti_op_push_activation
    IMPORT JITStubThunked_op_push_activation
cti_op_push_activation PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_push_activation
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_push_activation ENDP

    EXPORT cti_op_call_NotJSFunction
    IMPORT JITStubThunked_op_call_NotJSFunction
cti_op_call_NotJSFunction PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_call_NotJSFunction
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_call_NotJSFunction ENDP

    EXPORT cti_op_create_arguments
    IMPORT JITStubThunked_op_create_arguments
cti_op_create_arguments PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_create_arguments
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_create_arguments ENDP

    EXPORT cti_op_create_arguments_no_params
    IMPORT JITStubThunked_op_create_arguments_no_params
cti_op_create_arguments_no_params PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_create_arguments_no_params
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_create_arguments_no_params ENDP

    EXPORT cti_op_tear_off_activation
    IMPORT JITStubThunked_op_tear_off_activation
cti_op_tear_off_activation PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_tear_off_activation
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_tear_off_activation ENDP

    EXPORT cti_op_tear_off_arguments
    IMPORT JITStubThunked_op_tear_off_arguments
cti_op_tear_off_arguments PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_tear_off_arguments
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_tear_off_arguments ENDP

    EXPORT cti_op_profile_will_call
    IMPORT JITStubThunked_op_profile_will_call
cti_op_profile_will_call PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_profile_will_call
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_profile_will_call ENDP

    EXPORT cti_op_profile_did_call
    IMPORT JITStubThunked_op_profile_did_call
cti_op_profile_did_call PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_profile_did_call
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_profile_did_call ENDP

    EXPORT cti_op_new_array
    IMPORT JITStubThunked_op_new_array
cti_op_new_array PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_new_array
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_new_array ENDP

    EXPORT cti_op_resolve
    IMPORT JITStubThunked_op_resolve
cti_op_resolve PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve ENDP

    EXPORT cti_op_construct_NotJSConstruct
    IMPORT JITStubThunked_op_construct_NotJSConstruct
cti_op_construct_NotJSConstruct PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_construct_NotJSConstruct
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_construct_NotJSConstruct ENDP

    EXPORT cti_op_get_by_val
    IMPORT JITStubThunked_op_get_by_val
cti_op_get_by_val PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_val
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_val ENDP

    EXPORT cti_op_get_by_val_string
    IMPORT JITStubThunked_op_get_by_val_string
cti_op_get_by_val_string PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_val_string
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_val_string ENDP

    EXPORT cti_op_get_by_val_byte_array
    IMPORT JITStubThunked_op_get_by_val_byte_array
cti_op_get_by_val_byte_array PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_by_val_byte_array
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_by_val_byte_array ENDP

    EXPORT cti_op_sub
    IMPORT JITStubThunked_op_sub
cti_op_sub PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_sub
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_sub ENDP

    EXPORT cti_op_put_by_val
    IMPORT JITStubThunked_op_put_by_val
cti_op_put_by_val PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_val
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_val ENDP

    EXPORT cti_op_put_by_val_byte_array
    IMPORT JITStubThunked_op_put_by_val_byte_array
cti_op_put_by_val_byte_array PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_val_byte_array
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_val_byte_array ENDP

    EXPORT cti_op_lesseq
    IMPORT JITStubThunked_op_lesseq
cti_op_lesseq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_lesseq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_lesseq ENDP

    EXPORT cti_op_load_varargs
    IMPORT JITStubThunked_op_load_varargs
cti_op_load_varargs PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_load_varargs
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_load_varargs ENDP

    EXPORT cti_op_negate
    IMPORT JITStubThunked_op_negate
cti_op_negate PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_negate
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_negate ENDP

    EXPORT cti_op_resolve_base
    IMPORT JITStubThunked_op_resolve_base
cti_op_resolve_base PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve_base
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve_base ENDP

    EXPORT cti_op_resolve_base_strict_put
    IMPORT JITStubThunked_op_resolve_base_strict_put
cti_op_resolve_base_strict_put PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve_base_strict_put
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve_base_strict_put ENDP

    EXPORT cti_op_ensure_property_exists
    IMPORT JITStubThunked_op_ensure_property_exists
cti_op_ensure_property_exists PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_ensure_property_exists
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_ensure_property_exists ENDP

    EXPORT cti_op_resolve_skip
    IMPORT JITStubThunked_op_resolve_skip
cti_op_resolve_skip PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve_skip
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve_skip ENDP

    EXPORT cti_op_resolve_global
    IMPORT JITStubThunked_op_resolve_global
cti_op_resolve_global PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve_global
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve_global ENDP

    EXPORT cti_op_div
    IMPORT JITStubThunked_op_div
cti_op_div PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_div
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_div ENDP

    EXPORT cti_op_pre_dec
    IMPORT JITStubThunked_op_pre_dec
cti_op_pre_dec PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_pre_dec
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_pre_dec ENDP

    EXPORT cti_op_jless
    IMPORT JITStubThunked_op_jless
cti_op_jless PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_jless
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_jless ENDP

    EXPORT cti_op_jlesseq
    IMPORT JITStubThunked_op_jlesseq
cti_op_jlesseq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_jlesseq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_jlesseq ENDP

    EXPORT cti_op_not
    IMPORT JITStubThunked_op_not
cti_op_not PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_not
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_not ENDP

    EXPORT cti_op_jtrue
    IMPORT JITStubThunked_op_jtrue
cti_op_jtrue PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_jtrue
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_jtrue ENDP

    EXPORT cti_op_post_inc
    IMPORT JITStubThunked_op_post_inc
cti_op_post_inc PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_post_inc
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_post_inc ENDP

    EXPORT cti_op_eq
    IMPORT JITStubThunked_op_eq
cti_op_eq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_eq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_eq ENDP

    EXPORT cti_op_eq_strings
    IMPORT JITStubThunked_op_eq_strings
cti_op_eq_strings PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_eq_strings
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_eq_strings ENDP

    EXPORT cti_op_lshift
    IMPORT JITStubThunked_op_lshift
cti_op_lshift PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_lshift
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_lshift ENDP

    EXPORT cti_op_bitand
    IMPORT JITStubThunked_op_bitand
cti_op_bitand PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_bitand
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_bitand ENDP

    EXPORT cti_op_rshift
    IMPORT JITStubThunked_op_rshift
cti_op_rshift PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_rshift
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_rshift ENDP

    EXPORT cti_op_bitnot
    IMPORT JITStubThunked_op_bitnot
cti_op_bitnot PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_bitnot
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_bitnot ENDP

    EXPORT cti_op_resolve_with_base
    IMPORT JITStubThunked_op_resolve_with_base
cti_op_resolve_with_base PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_resolve_with_base
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_resolve_with_base ENDP

    EXPORT cti_op_new_func_exp
    IMPORT JITStubThunked_op_new_func_exp
cti_op_new_func_exp PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_new_func_exp
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_new_func_exp ENDP

    EXPORT cti_op_mod
    IMPORT JITStubThunked_op_mod
cti_op_mod PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_mod
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_mod ENDP

    EXPORT cti_op_less
    IMPORT JITStubThunked_op_less
cti_op_less PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_less
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_less ENDP

    EXPORT cti_op_post_dec
    IMPORT JITStubThunked_op_post_dec
cti_op_post_dec PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_post_dec
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_post_dec ENDP

    EXPORT cti_op_urshift
    IMPORT JITStubThunked_op_urshift
cti_op_urshift PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_urshift
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_urshift ENDP

    EXPORT cti_op_bitxor
    IMPORT JITStubThunked_op_bitxor
cti_op_bitxor PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_bitxor
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_bitxor ENDP

    EXPORT cti_op_new_regexp
    IMPORT JITStubThunked_op_new_regexp
cti_op_new_regexp PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_new_regexp
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_new_regexp ENDP

    EXPORT cti_op_bitor
    IMPORT JITStubThunked_op_bitor
cti_op_bitor PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_bitor
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_bitor ENDP

    EXPORT cti_op_call_eval
    IMPORT JITStubThunked_op_call_eval
cti_op_call_eval PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_call_eval
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_call_eval ENDP

    EXPORT cti_op_throw
    IMPORT JITStubThunked_op_throw
cti_op_throw PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_throw
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_throw ENDP

    EXPORT cti_op_get_pnames
    IMPORT JITStubThunked_op_get_pnames
cti_op_get_pnames PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_get_pnames
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_get_pnames ENDP

    EXPORT cti_has_property
    IMPORT JITStubThunked_has_property
cti_has_property PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_has_property
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_has_property ENDP

    EXPORT cti_op_push_scope
    IMPORT JITStubThunked_op_push_scope
cti_op_push_scope PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_push_scope
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_push_scope ENDP

    EXPORT cti_op_pop_scope
    IMPORT JITStubThunked_op_pop_scope
cti_op_pop_scope PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_pop_scope
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_pop_scope ENDP

    EXPORT cti_op_typeof
    IMPORT JITStubThunked_op_typeof
cti_op_typeof PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_typeof
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_typeof ENDP

    EXPORT cti_op_is_undefined
    IMPORT JITStubThunked_op_is_undefined
cti_op_is_undefined PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_undefined
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_undefined ENDP

    EXPORT cti_op_is_boolean
    IMPORT JITStubThunked_op_is_boolean
cti_op_is_boolean PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_boolean
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_boolean ENDP

    EXPORT cti_op_is_number
    IMPORT JITStubThunked_op_is_number
cti_op_is_number PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_number
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_number ENDP

    EXPORT cti_op_is_string
    IMPORT JITStubThunked_op_is_string
cti_op_is_string PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_string
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_string ENDP

    EXPORT cti_op_is_object
    IMPORT JITStubThunked_op_is_object
cti_op_is_object PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_object
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_object ENDP

    EXPORT cti_op_is_function
    IMPORT JITStubThunked_op_is_function
cti_op_is_function PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_is_function
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_is_function ENDP

    EXPORT cti_op_stricteq
    IMPORT JITStubThunked_op_stricteq
cti_op_stricteq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_stricteq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_stricteq ENDP

    EXPORT cti_op_to_primitive
    IMPORT JITStubThunked_op_to_primitive
cti_op_to_primitive PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_to_primitive
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_to_primitive ENDP

    EXPORT cti_op_strcat
    IMPORT JITStubThunked_op_strcat
cti_op_strcat PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_strcat
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_strcat ENDP

    EXPORT cti_op_nstricteq
    IMPORT JITStubThunked_op_nstricteq
cti_op_nstricteq PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_nstricteq
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_nstricteq ENDP

    EXPORT cti_op_to_jsnumber
    IMPORT JITStubThunked_op_to_jsnumber
cti_op_to_jsnumber PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_to_jsnumber
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_to_jsnumber ENDP

    EXPORT cti_op_in
    IMPORT JITStubThunked_op_in
cti_op_in PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_in
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_in ENDP

    EXPORT cti_op_push_new_scope
    IMPORT JITStubThunked_op_push_new_scope
cti_op_push_new_scope PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_push_new_scope
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_push_new_scope ENDP

    EXPORT cti_op_jmp_scopes
    IMPORT JITStubThunked_op_jmp_scopes
cti_op_jmp_scopes PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_jmp_scopes
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_jmp_scopes ENDP

    EXPORT cti_op_put_by_index
    IMPORT JITStubThunked_op_put_by_index
cti_op_put_by_index PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_by_index
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_by_index ENDP

    EXPORT cti_op_switch_imm
    IMPORT JITStubThunked_op_switch_imm
cti_op_switch_imm PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_switch_imm
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_switch_imm ENDP

    EXPORT cti_op_switch_char
    IMPORT JITStubThunked_op_switch_char
cti_op_switch_char PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_switch_char
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_switch_char ENDP

    EXPORT cti_op_switch_string
    IMPORT JITStubThunked_op_switch_string
cti_op_switch_string PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_switch_string
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_switch_string ENDP

    EXPORT cti_op_del_by_val
    IMPORT JITStubThunked_op_del_by_val
cti_op_del_by_val PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_del_by_val
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_del_by_val ENDP

    EXPORT cti_op_put_getter
    IMPORT JITStubThunked_op_put_getter
cti_op_put_getter PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_getter
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_getter ENDP

    EXPORT cti_op_put_setter
    IMPORT JITStubThunked_op_put_setter
cti_op_put_setter PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_put_setter
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_put_setter ENDP

    EXPORT cti_op_throw_reference_error
    IMPORT JITStubThunked_op_throw_reference_error
cti_op_throw_reference_error PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_throw_reference_error
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_throw_reference_error ENDP

    EXPORT cti_op_debug
    IMPORT JITStubThunked_op_debug
cti_op_debug PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_op_debug
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_op_debug ENDP

    EXPORT cti_vm_throw
    IMPORT JITStubThunked_vm_throw
cti_vm_throw PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_vm_throw
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_vm_throw ENDP

    EXPORT cti_to_object
    IMPORT JITStubThunked_to_object
cti_to_object PROC
    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bl JITStubThunked_to_object
    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET
    bx lr
cti_to_object ENDP

    END
