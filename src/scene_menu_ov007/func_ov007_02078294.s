/*
 * Overlay 7 trampoline to ov005 func_ov005_020663d8 (0x02078294-0x020782A4).
 * Calls ov005 element list flag setter with list index 2.
 */

.section .text
.global func_ov007_02078294
func_ov007_02078294:
    ldr ip, =0x020663D8
    mov r0, #2
    bx ip