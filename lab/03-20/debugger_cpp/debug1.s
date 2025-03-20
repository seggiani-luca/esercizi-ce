#include "libce.h"

.global a_debug
.extern c_debug

a_debug:
    salva_registri
    
    leaq 120(%rsp), %rdi
    call c_debug

    carica_registri
    iretq
