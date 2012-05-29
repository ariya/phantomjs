extern "C" EncodedJSValue JITStubThunked_op_create_this(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_create_this(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_create_this
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_create_this
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_convert_this(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_convert_this(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_convert_this
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_convert_this
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_convert_this_strict(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_convert_this_strict(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_convert_this_strict
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_convert_this_strict
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_add(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_add(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_add
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_add
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_pre_inc(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_pre_inc(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_pre_inc
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_pre_inc
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_timeout_check(STUB_ARGS_DECLARATION);
__asm int cti_timeout_check(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_timeout_check
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_timeout_check
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_register_file_check(STUB_ARGS_DECLARATION);
__asm void* cti_register_file_check(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_register_file_check
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_register_file_check
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_loop_if_lesseq(STUB_ARGS_DECLARATION);
__asm int cti_op_loop_if_lesseq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_loop_if_lesseq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_loop_if_lesseq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_new_object(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_new_object(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_new_object
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id_generic(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id_generic(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_generic
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id_direct_generic(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id_direct_generic(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_direct_generic
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_generic(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_generic(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_generic
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id_direct(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id_direct(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_direct
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id_fail(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_id_direct_fail(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_id_direct_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_direct_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_put_by_id_transition_realloc(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_put_by_id_transition_realloc(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_id_transition_realloc
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_transition_realloc
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_method_check(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_method_check(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_method_check
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_method_check
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_self_fail(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_self_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_self_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_self_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_getter_stub(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_getter_stub(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_getter_stub
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_getter_stub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_custom_stub(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_custom_stub(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_custom_stub
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_custom_stub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_proto_list(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_proto_list(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_proto_list
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_list
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_proto_list_full(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_proto_list_full(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_proto_list_full
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_list_full
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_proto_fail(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_proto_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_proto_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_array_fail(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_array_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_array_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_array_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_id_string_fail(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_id_string_fail(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_id_string_fail
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_string_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_check_has_instance(STUB_ARGS_DECLARATION);
__asm void cti_op_check_has_instance(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_check_has_instance
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_check_has_instance
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_instanceof(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_instanceof(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_instanceof
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_instanceof
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_del_by_id(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_del_by_id(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_del_by_id
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_del_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_mul(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_mul(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_mul
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_mul
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_new_func(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_new_func(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_new_func
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_func
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_call_jitCompile(STUB_ARGS_DECLARATION);
__asm void* cti_op_call_jitCompile(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_call_jitCompile
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_jitCompile
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_construct_jitCompile(STUB_ARGS_DECLARATION);
__asm void* cti_op_construct_jitCompile(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_construct_jitCompile
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_jitCompile
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_call_arityCheck(STUB_ARGS_DECLARATION);
__asm void* cti_op_call_arityCheck(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_call_arityCheck
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_arityCheck
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_construct_arityCheck(STUB_ARGS_DECLARATION);
__asm void* cti_op_construct_arityCheck(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_construct_arityCheck
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_arityCheck
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_vm_lazyLinkCall(STUB_ARGS_DECLARATION);
__asm void* cti_vm_lazyLinkCall(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_vm_lazyLinkCall
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_lazyLinkCall
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_vm_lazyLinkConstruct(STUB_ARGS_DECLARATION);
__asm void* cti_vm_lazyLinkConstruct(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_vm_lazyLinkConstruct
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_lazyLinkConstruct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_push_activation(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_push_activation(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_push_activation
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_activation
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_call_NotJSFunction(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_call_NotJSFunction(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_call_NotJSFunction
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_NotJSFunction
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_create_arguments(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_create_arguments(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_create_arguments
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_create_arguments
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_create_arguments_no_params(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_create_arguments_no_params(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_create_arguments_no_params
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_create_arguments_no_params
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_tear_off_activation(STUB_ARGS_DECLARATION);
__asm void cti_op_tear_off_activation(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_tear_off_activation
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_tear_off_activation
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_tear_off_arguments(STUB_ARGS_DECLARATION);
__asm void cti_op_tear_off_arguments(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_tear_off_arguments
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_tear_off_arguments
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_profile_will_call(STUB_ARGS_DECLARATION);
__asm void cti_op_profile_will_call(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_profile_will_call
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_profile_will_call
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_profile_did_call(STUB_ARGS_DECLARATION);
__asm void cti_op_profile_did_call(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_profile_did_call
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_profile_did_call
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_new_array(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_new_array(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_new_array
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_array
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_construct_NotJSConstruct(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_construct_NotJSConstruct(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_construct_NotJSConstruct
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_NotJSConstruct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_val(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_val(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_val
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_val_string(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_val_string(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_val_string
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val_string
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_get_by_val_byte_array(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_get_by_val_byte_array(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_by_val_byte_array
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val_byte_array
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_sub(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_sub(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_sub
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_sub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_val(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_val(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_val
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_val_byte_array(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_val_byte_array(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_val_byte_array
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_val_byte_array
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_lesseq(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_lesseq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_lesseq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_lesseq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_load_varargs(STUB_ARGS_DECLARATION);
__asm int cti_op_load_varargs(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_load_varargs
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_load_varargs
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_negate(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_negate(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_negate
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_negate
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve_base(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve_base(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve_base
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_base
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve_base_strict_put(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve_base_strict_put(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve_base_strict_put
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_base_strict_put
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_ensure_property_exists(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_ensure_property_exists(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_ensure_property_exists
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_ensure_property_exists
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve_skip(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve_skip(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve_skip
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_skip
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve_global(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve_global(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve_global
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_global
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_div(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_div(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_div
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_div
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_pre_dec(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_pre_dec(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_pre_dec
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_pre_dec
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_jless(STUB_ARGS_DECLARATION);
__asm int cti_op_jless(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_jless
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jless
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_jlesseq(STUB_ARGS_DECLARATION);
__asm int cti_op_jlesseq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_jlesseq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jlesseq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_not(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_not(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_not
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_not
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_jtrue(STUB_ARGS_DECLARATION);
__asm int cti_op_jtrue(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_jtrue
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jtrue
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_post_inc(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_post_inc(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_post_inc
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_post_inc
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_eq(STUB_ARGS_DECLARATION);
__asm int cti_op_eq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_eq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_eq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_op_eq_strings(STUB_ARGS_DECLARATION);
__asm int cti_op_eq_strings(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_eq_strings
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_eq_strings
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_lshift(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_lshift(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_lshift
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_lshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_bitand(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_bitand(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_bitand
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitand
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_rshift(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_rshift(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_rshift
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_rshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_bitnot(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_bitnot(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_bitnot
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitnot
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_resolve_with_base(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_resolve_with_base(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_resolve_with_base
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_with_base
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_new_func_exp(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_new_func_exp(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_new_func_exp
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_func_exp
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_mod(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_mod(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_mod
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_mod
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_less(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_less(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_less
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_less
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_post_dec(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_post_dec(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_post_dec
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_post_dec
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_urshift(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_urshift(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_urshift
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_urshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_bitxor(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_bitxor(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_bitxor
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitxor
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_new_regexp(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_new_regexp(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_new_regexp
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_regexp
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_bitor(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_bitor(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_bitor
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitor
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_call_eval(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_call_eval(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_call_eval
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_eval
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_throw(STUB_ARGS_DECLARATION);
__asm void* cti_op_throw(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_throw
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_throw
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSPropertyNameIterator* JITStubThunked_op_get_pnames(STUB_ARGS_DECLARATION);
__asm JSPropertyNameIterator* cti_op_get_pnames(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_get_pnames
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_pnames
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" int JITStubThunked_has_property(STUB_ARGS_DECLARATION);
__asm int cti_has_property(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_has_property
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_has_property
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_push_scope(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_push_scope(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_push_scope
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_pop_scope(STUB_ARGS_DECLARATION);
__asm void cti_op_pop_scope(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_pop_scope
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_pop_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_typeof(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_typeof(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_typeof
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_typeof
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_undefined(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_undefined(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_undefined
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_undefined
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_boolean(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_boolean(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_boolean
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_boolean
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_number(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_number(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_number
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_number
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_string(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_string(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_string
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_string
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_object(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_object(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_object
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_is_function(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_is_function(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_is_function
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_function
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_stricteq(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_stricteq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_stricteq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_stricteq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_to_primitive(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_to_primitive(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_to_primitive
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_to_primitive
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_strcat(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_strcat(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_strcat
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_strcat
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_nstricteq(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_nstricteq(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_nstricteq
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_nstricteq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_to_jsnumber(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_to_jsnumber(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_to_jsnumber
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_to_jsnumber
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_in(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_in(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_in
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_in
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" JSObject* JITStubThunked_op_push_new_scope(STUB_ARGS_DECLARATION);
__asm JSObject* cti_op_push_new_scope(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_push_new_scope
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_new_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_jmp_scopes(STUB_ARGS_DECLARATION);
__asm void cti_op_jmp_scopes(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_jmp_scopes
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jmp_scopes
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_by_index(STUB_ARGS_DECLARATION);
__asm void cti_op_put_by_index(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_by_index
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_index
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_switch_imm(STUB_ARGS_DECLARATION);
__asm void* cti_op_switch_imm(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_switch_imm
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_imm
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_switch_char(STUB_ARGS_DECLARATION);
__asm void* cti_op_switch_char(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_switch_char
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_char
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_op_switch_string(STUB_ARGS_DECLARATION);
__asm void* cti_op_switch_string(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_switch_string
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_string
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_op_del_by_val(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_op_del_by_val(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_del_by_val
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_del_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_getter(STUB_ARGS_DECLARATION);
__asm void cti_op_put_getter(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_getter
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_getter
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_put_setter(STUB_ARGS_DECLARATION);
__asm void cti_op_put_setter(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_put_setter
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_setter
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_throw_reference_error(STUB_ARGS_DECLARATION);
__asm void cti_op_throw_reference_error(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_throw_reference_error
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_throw_reference_error
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void JITStubThunked_op_debug(STUB_ARGS_DECLARATION);
__asm void cti_op_debug(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_op_debug
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_debug
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" void* JITStubThunked_vm_throw(STUB_ARGS_DECLARATION);
__asm void* cti_vm_throw(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_vm_throw
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_throw
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

extern "C" EncodedJSValue JITStubThunked_to_object(STUB_ARGS_DECLARATION);
__asm EncodedJSValue cti_to_object(STUB_ARGS_DECLARATION)
{
    PRESERVE8
    IMPORT JITStubThunked_to_object
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_to_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
}

