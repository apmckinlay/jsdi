;  Copyright 2014 (c) Suneido Software Corp. All rights reserved.
;  Licensed under GPLv2.


;===============================================================================
; file: invoke64_ll.asm
; auth: Victor Schappert
; date: 20140708
; desc: Windows x64 ABI invocation assembly file
;===============================================================================

; List of exported functions 
PUBLIC    invoke64_basic
PUBLIC    invoke64_fp

;===============================================================================
;                               invoke64_basic
;===============================================================================

comment @
    The "C" function signature is:
        uint64_t invoke64_basic(size_t args_size_bytes, const void * args_ptr,
                                void * func_ptr);

    The incoming register layout is therefore:
        - rcx : Contains the 'args_size_bytes' counter, which must be a multiple
                of 8 bytes or the program will crash
        - rdx : Contains the 'args_ptr' pointer
        - r8  : Contains the 'func_ptr' to invoke

    The only non-volatile register used by this function is 'rbp', which is used
    as a frame pointer and needs to be preserved across calls. The overall
    scheme for register use is:

        rbp              : Frame pointer
        rcx, rdx, r8, r9 : Arguments to invoked 'func_ptr'
        rax              : Cached value of 'func_ptr'
        r10              : Cached value of 'args_size_bytes' (goes to 0)
        r11              : Cached value of 'args_ptr'
@

_TEXT   SEGMENT

invoke64_basic      PROC FRAME

; Prologue

    push    rbp             ; Save non-volatile rbp; stack now 16-byte aligned
.pushreg rbp
    mov     rbp, rsp        ; Frame pointer
.setframe rbp, 0
    sub     rsp, 32         ; Alloc. enough stack for called func's homespace
.allocstack 32
.endprolog

; Function body

    mov     r10, rdx        ; Preserve 'args_ptr' in r10
    mov     r11, rcx        ; Preserve 'args_size_bytes' in r11
    mov     rax, r8         ; Preserve 'func_ptr' in rax

; Calculate jump table index ... The instructions below are computing a function
; f(x), where 'x' is the number of arguments to be passed to the invoked
; function and f(x) => (x % 4) + (x < 4 ? 0 : 4). This function has a range
; [0..7] where [0..3] indicate zero to three arguments; 4 indicates four
; or more arguments where 'x' is evenly divisible by four; and [5..7] indicate
; more than four arguments where 'x' is not evenly divisible by four. We use the
; index f(x) to jump to the appropriate argument handling label.

    shr     rcx, 3          ; Divide argument bytes by 8 to get count ('x')
    mov     rdx, rcx        ; Put count ('x') in rdx
    and     rdx, 3          ; Put (x % 4) in rdx
    cmp     rcx, 4
    lea     r8, [rdx+4]     ; Compute (x % 4) + 4 and store in r8
    cmovae  rdx, r8         ; Conditional move r8 into rdx if rcx ('x') > 4

; Jump based on the index f(x) contained in rdx

    lea     rcx, [Lbasic_jumptbl]
    jmp     QWORD PTR [rcx+rdx*8]

; Simple case: receive 0-3 arguments which must be passed in registers

Lbasic_3args:               ; Invoked with 3 arguments
    mov     r8, [r10+16]
Lbasic_2args:               ; Invoked with 2 arguments
    mov     rdx, [r10+8]
Lbasic_1args:               ; Invoked with 1 argument
    mov     rcx, [r10]
Lbasic_0args:               ; Invoked with no arguments
    call    rax
    jmp     Lbasic_epilogue

; General case: receive 4-N arguments where the first four need to be passed in
; registers.

Lbasic_mod3args:            ; Invoked with 7, 11, 15, etc. arguments
    sub     r11, 8
    mov     r8, [r10+r11]
Lbasic_mod2args:            ; Invoked with 6, 10, 14, etc. arguments
    sub     r11, 8
    mov     rdx, [r10+r11]
Lbasic_mod1args:            ; Invoked with 5, 9, 13, etc. arguments
    sub     r11, 8
    mov     rcx, [r10+r11]
Lbasic_loop:
    mov     [rsp+24], r9    ; Shift excess args from register to stack
    mov     [rsp+16], r8    ; Some registers may contain garbage the first
    mov     [rsp+8], rdx    ; iteration...
    mov     [rsp], rcx
    sub     rsp, 32         ; Allocate 32 more bytes of stack
Lbasic_mod0args:            ; Invoked with 4, 8, 12, 16, etc. arguments
    mov     r9, [r10+r11-8] ; Put next four arguments into argument registers
    mov     r8, [r10+r11-16]
    mov     rdx, [r10+r11-24]
    mov     rcx, [r10+r11-32]
    sub     r11, 32         ; Handled 32 bytes more args (NOTE 'sub' sets ZF)
    jnz     Lbasic_loop     ; Loop if more arguments remain
    call    rax

; Epilogue

Lbasic_epilogue:

    mov     rsp, rbp        ; Restore stack pointer from frame pointer
    pop     rbp             ; Restore saved value of non-volatile rbp
    ret

; Jump table

Lbasic_jumptbl:

    DQ      Lbasic_0args    ; f(x) = 0
    DQ      Lbasic_1args    ; f(x) = 1
    DQ      Lbasic_2args    ; f(x) = 2
    DQ      Lbasic_3args    ; f(x) = 3
    DQ      Lbasic_mod0args ; f(x) = 4
    DQ      Lbasic_mod1args ; f(x) = 5
    DQ      Lbasic_mod2args ; f(x) = 6
    DQ      Lbasic_mod3args ; f(x) = 7
    
invoke64_basic  ENDP

_TEXT ENDS

;===============================================================================
;                                 invoke64_fp
;===============================================================================

comment @
    The "C" function signature is:
        uint64_t invoke64_fp(size_t args_size_bytes, const void * args_ptr,
                             void * func_ptr, uint8_t register_types);

    The incoming register layout is therefore:
        - rcx : Contains the 'args_size_bytes' counter, which must be a multiple
                of 8 bytes or the program will crash
        - rdx : Contains the 'args_ptr' pointer
        - r8  : Contains the 'func_ptr' to invoke
        - r9  : Contains the 'register_types' encoded in the low-order 8 bits

    The only non-volatile register used by this function is 'rbp', which is used
    as a frame pointer and needs to be preserved across calls. The overall
    scheme for register use is:

        rbp              : Frame pointer
        rcx, rdx, r8, r9 : Arguments to invoked 'func_ptr'
        xmm4             : Cached value of 'func_ptr'
        r10              : Cached value of 'args_size_bytes' (goes to 0)
        r11              : Cached value of 'args_ptr'
        rax              : Cached value of 'register_types'
@

_TEXT   SEGMENT

invoke64_fp     PROC FRAME

; Prologue

    push    rbp             ; Save non-volatile rbp; stack now 16-byte aligned
.pushreg rbp
    mov     rbp, rsp        ; Frame pointer
.setframe rbp, 0
    sub     rsp, 32         ; Alloc. enough stack for called func's homespace
.allocstack 32
.endprolog

; Function body

    mov     r10, rdx        ; Preserve 'args_ptr' in r10
    mov     r11, rcx        ; Preserve 'args_size_bytes' in r11
    movd    xmm4, r8        ; Preserve 'func_ptr' in xmm4
                            ;     http://stackoverflow.com/q/24789339/1911388
    mov     rax, r9         ; Preserve 'register_types' in rax

; Calculate jump table index ... The instructions below are computing a function
; g(x), where 'x' is the number of arguments to be passed to the invoked
; function and g(x) => (x % 4) + (x <= 4 ? 0 : 4). This function has a range
; [0..8] where [0..4] indicates zero to four arguments; 8 indicates more than
; four arguments where 'x' is evenly divisible by four; and [5..7] indicate more
; than four arguments where 'x' is not evenly divisible by four. We use the
; index g(x) to jump to the appropriate argument handling label.

    shr     rcx, 3          ; Divide argument bytes by 8 to get count ('x')

; Because g(x : x <= 8) => x, you can jump directly if x <= 8.

    cmp     rcx, 8
    jle     Lfp_jump

; Otherwise, compute g(x : 8 < x).

    mov     r9, rcx         ; r9 := 'x'
    and     r9, 3           ; r9 := (x % 4)
    lea     rcx, [r9+4]     ; rcx := (x % 4) + 4
    test    r9, r9          ; zf := !((x % 4) + 4)
    mov     edx, 8
    cmovz   ecx, edx        ; Conditional move 8 into rcx if !((x % 4)+4)

; Jump based on the index g(x) contained in rcx

Lfp_jump:    
    lea     rdx, [Lfp_jumptbl]
    jmp     QWORD PTR [rdx+rcx*8]

; Loop for x >= 4. 

Lfp_loop:
Lfp_jtbl_8:
    sub     r11, 8
    mov     r9, [r10+r11]
    mov     [rsp+24], r9
Lfp_jtbl_7:
    sub     r11, 8
    mov     r8, [r10+r11]
    mov     [rsp+16], r8
Lfp_jtbl_6:
    sub     r11, 8
    mov     rdx, [r10+r11]
    mov     [rsp+8], rdx
Lfp_jtbl_5:
    sub     r11, 8
    mov     rcx, [r10+r11]
    mov     [rsp], rcx
    sub     rsp, 32
    cmp     r11, 32
    jg      Lfp_loop
Lfp_jtbl_4:
    dec     al
    jz      Lfp_4double
    jg      Lfp_4float
    mov     r9, [r10+24]
    shr     rax, 8
    dec     al
    jz      Lfp_3double
    jg      Lfp_3float
Lfp_3int:
    mov     r8, [r10+16]
    shr     rax, 8
    dec     al
    jz      Lfp_2double
    jg      Lfp_2float
Lfp_2int:
    mov     rdx, [r10+8]
    shr     rax, 8
    dec     al
    jz      Lfp_1double
    jg      Lfp_1float
Lfp_1int:
    mov     rcx, [r10]
    jmp     Lfp_jtbl_0

; Out-of-loop jump targets for 1..3 arguments (x in [1..3])
    
Lfp_jtbl_3:
    shr     rax, 8
    dec     al
    jl      Lfp_3int
    jz      Lfp_3double
    jmp     Lfp_3float
Lfp_jtbl_2:
    shr     rax, 16
    dec     al
    jl      Lfp_2int
    jz      Lfp_2double
    jmp     Lfp_2float
Lfp_jtbl_1:
    shr     rax, 24
    dec     al
    jl      Lfp_1int
    jz      Lfp_1double
    jmp     Lfp_1float

; Handlers for putting "double" values into the appropriate xmm registers.

Lfp_4double:
    movsd   xmm3, QWORD PTR [r10+24]
    shr     rax, 8
    dec     al
    jl      Lfp_3int
    jg      Lfp_3float
Lfp_3double:
    movsd   xmm2, QWORD PTR [r10+16]
    shr     rax, 8
    dec     al
    jl      Lfp_2int
    jg      Lfp_2float
Lfp_2double:
    movsd   xmm1, QWORD PTR [r10+8]
    shr     rax, 8
    dec     al
    jl      Lfp_1int
    jg      Lfp_1float
Lfp_1double:
    movsd   xmm0, QWORD PTR [r10]
    jmp     Lfp_jtbl_0

; Handlers for putting "single" values into the appropriate xmm registers.

Lfp_4float:
    movss   xmm3, DWORD PTR [r10+24]
    shr     rax, 8
    dec     al
    jl      Lfp_3int
    jz      Lfp_3double
Lfp_3float:
    movss   xmm2, DWORD PTR [r10+16]
    shr     rax, 8
    dec     al
    jl      Lfp_2int
    jz      Lfp_2double
Lfp_2float:
    movss   xmm1, DWORD PTR [r10+8]
    shr     rax, 8
    dec     al
    jl      Lfp_1int
    jz      Lfp_1double
Lfp_1float:    
    movss   xmm0, DWORD PTR [r10]

; Call the function

Lfp_jtbl_0:
    movd    rax, xmm4       ; http://stackoverflow.com/q/24789339/1911388
    call    rax

; Epilogue

Lfp_epilogue:

    mov     rsp, rbp        ; Restore stack pointer from frame pointer
    pop     rbp             ; Restore saved value of non-volatile rbp
    ret

; Jump table

Lfp_jumptbl:

    DQ      Lfp_jtbl_0      ; g(x) = 0
    DQ      Lfp_jtbl_1      ; g(x) = 1
    DQ      Lfp_jtbl_2      ; g(x) = 2
    DQ      Lfp_jtbl_3      ; g(x) = 3
    DQ      Lfp_jtbl_4      ; g(x) = 4
    DQ      Lfp_jtbl_5      ; g(x) = 5
    DQ      Lfp_jtbl_6      ; g(x) = 6
    DQ      Lfp_jtbl_7      ; g(x) = 7
    DQ      Lfp_jtbl_8      ; g(x) = 8
    
invoke64_fp     ENDP

_TEXT ENDS

END
