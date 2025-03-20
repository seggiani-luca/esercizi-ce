#include "libce.h"

.global a_debug
.global a_single_step

.extern c_debug
.extern c_single_step

a_debug:
    salva_registri
    
    leaq 120(%rsp), %rdi
    call c_debug

    carica_registri
    orw $0x100, 16(%rsp) // presumo attivi la single step
    iretq

a_single_step:
    salva_registri
    
    call c_single_step

    carica_registri
    andw $0xFEFF, 16(%rsp) // presumo disattivi la single step
    iretq
