/*
 * FieldRotation_StartTimed (ov000, 0x020A411C-0x020A4214).
 * Starts a timed rotation transform on a field entity.
 * Includes conditional early exit (NE condition) at entry.
 */

        .section .text
        .arm
        .align 2

        .extern _s32_div_f

        .global FieldRotation_StartTimed
FieldRotation_StartTimed:
        /* Conditional early exit (NE condition) - 0x020A411C */
        /* if Z flag clear (NE): load and store, then return */
        ldrne   r1, [r1, #0x24]
        ldrne   r0, [r0, #0x1E0]
        /* MWCC doesn't support strhne, use branch around */
        bne     early_exit_store
        bx      lr
early_exit_store:
        strh    r1, [r0, #0x78]
        bx      lr

        /* Main function body - 0x020A412C */
        stmdb   sp!, {r4, r5, r6, r7, lr}
        sub     sp, sp, #4
        ldr     r5, [sp, #0x20]
        ldr     r4, [r0, #0x1E0]
        cmp     r5, #0
        addeq   r5, r0, #0x280
        ldrh    lr, [r4, #0x78]
        cmp     r1, #0
        bne     loc_020A4184_

        /* ldr r0, [pc, #0xB8] -> loads 0xFFFF0000 from pool at offset 0xF4 */
        ldr     r0, [pc, #0xB8]
        ldr     r1, [sp, #0x18]
        and     ip, r2, r0
        cmp     r1, #0
        subgt   r4, ip, lr
        mov     r0, #0x10000
        suble   r4, lr, ip
        rsb     r0, r0, #0
        cmp     r4, #0
        addlt   r4, r4, #0x10000
        and     r0, r2, r0
        add     r4, r4, r0
        b       loc_020A4198_

loc_020A4184_:
        ldr     r1, [sp, #0x18]
        /* ldr r0, [pc, #0x80] -> loads 0xFFFF0000 from pool at offset 0xF4 */
        ldr     r0, [pc, #0x80]
        mla     r1, r2, r1, lr
        mov     r4, r2
        and     ip, r1, r0
        ldrh    r2, [r5]
        ldr     r0, [sp, #0x18]
        ldrb    r1, [sp, #0x1C]
        orr     r2, r2, #4
        strh    r2, [r5]
        ldrh    r2, [r5]
        mov     r6, #0
        mul     r0, r4, r0
        bic     r2, r2, #0x10
        strh    r2, [r5]
        ldrh    r7, [r5]
        and     r2, r1, #1
        mov     r1, r3
        orr     r7, r7, #0x40
        strh    r7, [r5]
        str     r6, [r5, #0x2C]
        str     lr, [r5, #0x1C]
        str     ip, [r5, #0x24]
        strh    r3, [r5, #6]
        ldrh    r3, [r5]
        bic     r3, r3, #0x80
        orr     r2, r3, r2, lsl #7
        strh    r2, [r5]

loc_020A4198_:
        /* Call _s32_div_f via literal pool */
        ldr     ip, [pc, #0x18]
        blx     ip

        str     r0, [r5, #0x10]
        str     r4, [r5, #0x14]
        mov     r0, #1
        add     sp, sp, #4
        ldmia   sp!, {r4, r5, r6, r7, pc}

        /* Literal pool */
        .word   0x0000FFFF          /* 0xFFFF0000 for and operations */
        .word   _s32_div_f          /* Address of _s32_div_f */
        .align 2