.global _start
.extern mutate_self_text

.text
_start:
    subq $0x28, %rsp         
    movq %rsp, %rcx          
    call mutate_self_text
    addq $0x28, %rsp
    ret
