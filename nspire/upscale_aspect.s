/*
 * 240x160 -> 320x213 upscaler for ARM with interpolation
 *
 * Written by Gražvydas "notaz" Ignotas
 * Prototyped by Rokas
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  0   1   2 :  3   4   5
 *  6   7   8 :  9  10  11
 * 12  13  14 : 15  16  17
 *             v
 *  0   1   2   3 :  4   5   6   7
 *  8   9  10  11 : 12  13  14  15
 * 16  17  18  19 : 20  21  22  23
 * 24  25  26  27 : 28  29  30  31
 */

.macro unpack_hi dst, src
    mov     \dst, \src,       lsr #16
    orr     \dst, \dst, \dst, lsl #16
    and     \dst, \dst, lr
.endm

.macro unpack_lo dst, src
    mov     \dst, \src,       lsl #16
    orr     \dst, \dst, \dst, lsr #16
    and     \dst, \dst, lr
.endm

@ do 3:5 summing: r2 = (s1*3 + s2*5 + 4) / 8
@ s2 != r2
.macro do_3_5 s1, s2
    add     r2,\s1,\s1, lsl #1  @ r2  = s1 * 3
    add     r2, r2,\s2, lsl #2
    add     r2, r2,\s2          @ r2 += s2 * 5
    add     r2, r2, r12,lsl #2  @ sum += round * 4
    and     r2, lr, r2, lsr #3  @ mask_to_unpacked(sum / 8)
.endm

@ do 14:7:7:4: r2 = (s1*14 + s2*7 + s3*7 + s4*4 + 16) / 32
@ {s2,s3,s4} != r2
.macro do_14_7_7_4 s1, s2, s3, s4
	add     r2,\s2,\s3
	add     r2, r2,\s1, lsl #1  @ r2  = s1 * 2 + s2 + s3
	rsb     r2, r2, r2, lsl #3  @ r2 *= 7
    add     r2, r2,\s4, lsl #2	@ r2 += s4 * 4
    add     r2, r2, r12,lsl #4	@ sum += round * 16
    and     r2, lr, r2, lsr #5  @ mask_to_unpacked(sum / 32)
.endm

.global upscale_aspect @ u16 *dst, u16 *src
upscale_aspect:
    stmfd   sp!,{r4-r11,lr}
    mov     lr,     #0x0000001f
    orr     lr, lr, #0x00007c00 @ for "unpacked" form of
    orr     lr, lr, #0x03e00000	@ 000000gg'ggg00000'0bbbbb00'000rrrrr
    mov     r12,    #0x00000001
    orr     r12,r12,#0x00000400
    orr     r12,r12,#0x00200000 @ rounding constant

    mov     r8,     #((240/6)-1) << 24 @ cols
    orr     r8, r8, #160/3	@ rows

    add     r0, r0, #320*2*13
loop1:
    ldr     r10,[r1]
    ldr     r11,[r1, #320*2*1]

    unpack_lo r4, r10
    unpack_hi r5, r10
    unpack_lo r6, r11
    unpack_hi r7, r11

    ldr     r11,[r1, #4]

    do_3_5  r4, r5
    orr     r2, r2, r2, lsr #16
    mov     r3, r10,    lsl #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0]		  @ 0,1

    ldr     r10,[r1, #320*2*2]

    do_3_5  r4, r6
    orr     r3, r2, r2, lsl #16
    mov     r3, r3,     lsr #16	  @ 8

    do_14_7_7_4 r7, r5, r6, r4
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*1]	  @ 8,9

    unpack_lo r4, r10
    unpack_hi r9, r10

    do_3_5  r4, r6
    orr     r3, r2, r2, lsl #16
    mov     r3, r3,     lsr #16

    do_14_7_7_4 r7, r9, r6, r4
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*2]	  @ 16,17

    do_3_5  r4, r9
    orr     r2, r2, r2, lsr #16
    mov     r3, r10,    lsl #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*3]	  @ 24,25

    ldr     r10,[r1, #320*2*1+4]

    unpack_lo r6, r11
    unpack_lo r4, r10

    do_3_5  r6, r5
    orr     r2, r2, r2, lsl #16
    mov     r3, r11,    lsl #16
    orr     r2, r3, r2, lsr #16
    str     r2, [r0, #4]	  @ 2,3

    do_14_7_7_4 r7, r4, r5, r6
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    ldr     r5, [r1, #320*2*2+4]

    do_3_5  r6, r4
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*1+4]  @ 10,11

    unpack_lo r6, r5

    do_14_7_7_4 r7, r4, r9, r6
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    do_3_5  r6, r4
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*2+4]  @ 18,19

    unpack_hi r4, r10

    ldr     r10,[r1, #8]

    do_3_5  r6, r9
    orr     r2, r2, r2, lsl #16
    mov     r2, r2,     lsr #16
    orr     r2, r2, r5, lsl #16
    str     r2, [r0, #320*2*3+4]  @ 26,27

    unpack_hi r6, r11
    unpack_lo r7, r10

    do_3_5  r6, r7
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r11,lsr #16
    str     r2, [r0, #8]	  @ 4,5

    ldr     r11,[r1, #320*2*1+8]

    unpack_hi r9, r10

    do_3_5  r9, r7
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r10,lsr #16
    mov     r2, r2,     ror #16
    str     r2, [r0, #12]	  @ 6,7

    unpack_lo r10,r11

    do_3_5  r6, r4
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    do_14_7_7_4 r10, r4, r7, r6
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*1+8]  @ 12,13

    unpack_hi r6, r11

    ldr     r11,[r1, #320*2*2+8]

    do_14_7_7_4 r10, r6, r7, r9
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    do_3_5  r9, r6
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*1+12] @ 14,15

    unpack_hi r7, r5
    unpack_lo r9, r11

    do_3_5  r7, r4
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    do_14_7_7_4 r10, r4, r9, r7
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*2+8]  @ 20,21

    do_3_5  r7, r9
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r5, lsr #16
    str     r2, [r0, #320*2*3+8]  @ 28,29

    unpack_hi r5, r11

    do_14_7_7_4 r10, r6, r9, r5
    orr     r2, r2, r2, lsl #16
    mov     r3, r2,     lsr #16

    do_3_5  r5, r6
    orr     r2, r2, r2, lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #320*2*2+12] @ 22,23

    do_3_5  r5, r9
    orr     r2, r2, r2, lsr #16
    mov     r3, r11,    lsr #16
    orr     r2, r3, r2, lsl #16
    mov     r2, r2,     ror #16
    str     r2, [r0, #320*2*3+12]  @ 30,31

    add     r0, r0, #16
    add     r1, r1, #12

    subs    r8, r8, #1<<24
    bpl     loop1

    add     r0, r0, #320*3*2
    add     r1, r1, #(320*2+80)*2
    sub     r8, r8, #1
    tst     r8, #0xff
    add     r8, r8, #(240/6) << 24 @ cols
    bne     loop1

    @@ last line
    mov     r8, #240/6

loop2:
    ldmia   r1!,{r9,r10,r11}

    unpack_lo r4, r9
    unpack_hi r5, r9

    do_3_5  r4, r5
    orr     r2, r2, r2, lsr #16
    mov     r3, r9,     lsl #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0], #4

    unpack_lo r6, r10
    unpack_hi r7, r10

    do_3_5  r6, r5
    orr     r2, r2, r2, lsl #16
    mov     r2, r2,     lsr #16
    orr     r2, r2, r10,lsl #16
    str     r2, [r0], #4

    unpack_lo r4, r11
    unpack_hi r5, r11

    do_3_5  r7, r4
    orr     r2, r2, r2, lsr #16
    mov     r3, r10,    lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0], #4

    do_3_5  r5, r4
    orr     r2, r2, r2, lsr #16
    mov     r3, r11,    lsr #16
    orr     r2, r3, r2, lsl #16
    mov     r2, r2,     ror #16
    str     r2, [r0], #4

    subs    r8, r8, #1
    bne     loop2

    ldmfd   sp!,{r4-r11,pc}

.macro do_1_1 s1, s2
	eor r12, \s1, \s2
	and r12, r11, r12, lsr #1
	and \s1, \s1, \s2
	add \s1, \s1, r12
.endm
	
.global upscale_aspect_fast @ u16 *dst, u16 *src
upscale_aspect_fast:
	stmfd sp!, {r4-r11, lr}
	
	ldr r11, =0x3def3def
	mov lr, #80
loop_fast_1:
	sub lr, lr, #(40 << 26) + 1
loop_fast_2:
	add r12, r1, #320*2
	ldmia r12, {r6, r8, r9}
	ldmia r1!, {r2, r4, r5}
	
	mov r12, r8, lsl #16
	orr r10, r12, r12, lsr #16
	orr r7, r12, r6, lsr #16
	
	do_1_1 r7, r10
	
	mov r8, r8, lsr #16
	orr r8, r8, r9, lsl #16
	mov r10, r9, lsr #16
	orr r10, r10, r10, lsl #16
	
	do_1_1 r9, r10
	
	add r12, r0, #320*2 * 2
	stmia r12, {r6, r7, r8, r9}
	
	mov r12, r4, lsl #16
	orr r10, r12, r12, lsr #16
	orr r3, r12, r2, lsr #16
	
	do_1_1 r3, r10
	
	mov r4, r4, lsr #16
	orr r4, r4, r5, lsl #16
	mov r10, r5, lsr #16
	orr r10, r10, r10, lsl #16
	
	do_1_1 r5, r10
	
	do_1_1 r6, r2
	do_1_1 r7, r3
	do_1_1 r8, r4
	do_1_1 r9, r5
	
	add r12, r0, #320*2
	stmia r12, {r6, r7, r8, r9}
	
	adds lr, lr, #1 << 26
	
	stmia r0!, {r2, r3, r4, r5}
	
	bcc loop_fast_2
	
	add r0, r0, #320*2 * 3 - 320*2
	add r1, r1, #320*2 * 2 - 240*2
	
	bne loop_fast_1
	
	ldmfd sp!, {r4-r11, pc}
	
.global upscale_aspect_raw @ u16 *dst, u16 *src
upscale_aspect_raw:
	stmfd sp!,{r4-r11,lr}
	add r0,r0,#320*2*13
	mov r10,#54
	
loop_raw_1:
	mov r11,#240/12
loop_raw_2:
	ldmia r1!,{r2,r4,r5,r6,r8,r9}
	mov r3,r2,lsr #16
	orr r3,r3,r4,lsl #16
	mov r4,r4,lsr #16
	orr r4,r4,r5,lsl #16
	stmia r0!,{r2-r5}
	mov r7,r6,lsr #16
	orr r7,r7,r8,lsl #16
	mov r8,r8,lsr #16
	orr r8,r8,r9,lsl #16
	stmia r0!,{r6-r9}
	subs r11,r11,#1
	bne loop_raw_2
	add r1,r1,#320*2 - 240*2
	
	mov r11,#240/12
loop_raw_3:
	ldmia r1!,{r2,r4,r5,r6,r8,r9}
	mov r3,r2,lsr #16
	orr r3,r3,r4,lsl #16
	mov r4,r4,lsr #16
	orr r4,r4,r5,lsl #16
	stmia r0!,{r2-r5}
	mov r7,r6,lsr #16
	orr r7,r7,r8,lsl #16
	mov r8,r8,lsr #16
	orr r8,r8,r9,lsl #16
	stmia r0!,{r6-r9}
	subs r11,r11,#1
	bne loop_raw_3
	subs r10,r10,#1
	ldmeqfd sp!,{r4-r11,pc}
	add r1,r1,#320*2 - 240*2
	
	add r12,r0,#320*2
	mov r11,#240/12
loop_raw_4:
	ldmia r1!,{r2,r4,r5,r6,r8,r9}
	mov r3,r2,lsr #16
	orr r3,r3,r4,lsl #16
	mov r4,r4,lsr #16
	orr r4,r5,lsl #16
	stmia r0!,{r2-r5}
	stmia r12!,{r2-r5}
	mov r7,r6,lsr #16
	orr r7,r7,r8,lsl #16
	mov r8,r8,lsr #16
	orr r8,r8,r9,lsl #16
	stmia r0!,{r6-r9}
	stmia r12!,{r6-r9}
	subs r11,r11,#1
	bne loop_raw_4
	add r1,r1,#320*2 - 240*2
	mov r0,r12
	
	b loop_raw_1
	
/*
.global upscale_aspect_row @ void *dst, void *linesx4, u32 row
upscale_aspect_row:
    stmfd   sp!,{r4-r11,lr}
    mov     lr,     #0x0000001f
    orr     lr, lr, #0x0000f800 @ for "unpacked" form of
    orr     lr, lr, #0x07e00000	@ 00000ggg'ggg00000'rrrrr000'000bbbbb
    mov     r12,    #0x00000001
    orr     r12,r12,#0x00000800
    orr     r12,r12,#0x00200000 @ rounding constant

    mov     r8,     #(240/6)	@ cols

    add     r0, r0, #(240*320)*2
    add     r0, r0, #12*2
    add     r0, r0, r2, lsl #3

uar_loop:
    ldr     r10,[r1]
    ldr     r11,[r1, #240*2*1]

    unpack_lo r4, r10
    unpack_hi r5, r10
    unpack_lo r6, r11
    unpack_hi r7, r11

    ldr     r11,[r1, #240*2*2]

    do_3_5  r4, r6
    orr     r2, r2, r2, lsr #16
    mov     r3, r10,    lsl #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2]!	@ 0,8

    unpack_lo r10,r11
    unpack_hi r9, r11

    do_3_5  r10,r6
    orr     r2, r2, r2, lsl #16
    mov     r3, r11,    lsl #16
    orr     r2, r3, r2, lsr #16
    str     r2, [r0, #4]	@ 16,24

    do_3_5  r4, r5
    orr     r3, r2, r2, lsl #16

    do_14_7_7_4 r7, r5, r6, r4
    orr     r2, r2, r2, lsr #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2]!	@ 1,9

    ldr     r11,[r1, #4]

    do_14_7_7_4 r7, r6, r9, r10
    orr     r3, r2, r2, lsl #16

    do_3_5  r10,r9
    orr     r2, r2, r2, lsr #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #4]	@ 17,25

    ldr     r10,[r1, #240*2*1+4]

    unpack_lo r4, r11
    unpack_lo r6, r10

    do_3_5  r4, r5
    orr     r3, r2, r2, lsl #16

    do_14_7_7_4 r7, r5, r6, r4
    orr     r2, r2, r2, lsr #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2]!	@ 2,10

    do_3_5  r4, r6

    ldr     r4, [r1, #240*2*2+4]

    orr     r2, r2, r2, lsr #16
    mov     r3, r11,    lsl #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2]	@ 3,11

    unpack_lo r5, r4

    do_14_7_7_4 r7, r6, r9, r5
    orr     r3, r2, r2, lsl #16

    do_3_5  r5, r9
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r3, lsr #16
    str     r2, [r0, #4]	@ 18,26

    do_3_5  r5, r6
    orr     r2, r2, r2, lsl #16
    mov     r3, r4,     lsl #16
    orr     r2, r3, r2, lsr #16
    str     r2, [r0, #-240*2+4]	@ 19,27

    unpack_hi r5, r11
    unpack_hi r6, r10
    unpack_hi r7, r4

    ldr     r10,[r1, #8]

    do_3_5  r5, r6
    orr     r2, r2, r2, lsr #16
    mov     r3, r11,    lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2*2]! @ 4,12

    ldr     r11,[r1, #240*2*1+8]

    do_3_5  r7, r6
    orr     r2, r2, r2, lsl #16
    mov     r3, r4,     lsr #16
    mov     r3, r3,     lsl #16
    orr     r2, r3, r2, lsr #16
    str     r2, [r0, #4]	@ 20,28

    unpack_lo r4, r10
    unpack_lo r9, r11

    ldr     r11,[r1, #240*2*2+8]

    do_3_5  r5, r4
    orr     r3, r2, r2, lsl #16

    do_14_7_7_4 r9, r4, r6, r5
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r3, lsr #16
    str     r2, [r0, #-240*2]!	@ 5,13

    unpack_lo r5, r11

    do_14_7_7_4 r9, r5, r6, r7
    orr     r3, r2, r2, lsl #16

    do_3_5  r7, r5
    orr     r2, r2, r2, lsr #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #4]	@ 21,29

    ldr     r7, [r1, #240*2*1+8]

    unpack_hi r6, r10
    unpack_hi r7, r7

    do_3_5  r6, r4
    orr     r3, r2, r2, lsl #16

    do_14_7_7_4 r9, r4, r7, r6
    orr     r2, r2, r2, lsr #16
    mov     r2, r2,     lsl #16
    orr     r2, r2, r3, lsr #16
    str     r2, [r0, #-240*2]!	@ 6,14

    unpack_hi r4, r11

    do_14_7_7_4 r9, r5, r7, r4
    orr     r3, r2, r2, lsl #16

    do_3_5  r4, r5
    orr     r2, r2, r2, lsr #16
    mov     r3, r3,     lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #4]	@ 22,30

    do_3_5  r6, r7
    orr     r2, r2, r2, lsr #16
    mov     r3, r10,    lsr #16
    orr     r2, r3, r2, lsl #16
    str     r2, [r0, #-240*2]!	@ 7,15

    do_3_5  r4, r7
    orr     r2, r2, r2, lsl #16
    mov     r3, r11,    lsr #16
    mov     r3, r3,     lsl #16
    orr     r2, r3, r2, lsr #16
    str     r2, [r0, #4]	@ 23,31

    subs    r8, r8, #1
    add     r1, r1, #12
    bne     uar_loop

    ldmfd   sp!,{r4-r11,pc}


@ bonus function

@ input: r2-r5
@ output: r7,r8
@ trash: r6
.macro rb_line_low
    mov     r6, r2, lsl #16
    mov     r7, r3, lsl #16
    orr     r7, r7, r6, lsr #16
    mov     r6, r4, lsl #16
    mov     r8, r5, lsl #16
    orr     r8, r8, r6, lsr #16
.endm

.macro rb_line_hi
    mov     r6, r2, lsr #16
    mov     r7, r3, lsr #16
    orr     r7, r6, r7, lsl #16
    mov     r6, r4, lsr #16
    mov     r8, r5, lsr #16
    orr     r8, r6, r8, lsl #16
.endm

.global do_rotated_blit @ void *dst, void *linesx4, u32 y
do_rotated_blit:
    stmfd   sp!,{r4-r8,lr}

    add     r0, r0, #(240*320)*2
    sub     r0, r0, #(240*40)*2
    sub     r0, r0, #(240-40+4)*2	@ y starts from 4
    add     r0, r0, r2, lsl #1

    mov     lr, #240/4

rotated_blit_loop:
    ldr     r2, [r1, #240*0*2]
    ldr     r3, [r1, #240*1*2]
    ldr     r4, [r1, #240*2*2]
    ldr     r5, [r1, #240*3*2]
    rb_line_low
    stmia   r0, {r7,r8}
    sub     r0, r0, #240*2
    rb_line_hi
    stmia   r0, {r7,r8}
    sub     r0, r0, #240*2

    ldr     r2, [r1, #240*0*2+4]
    ldr     r3, [r1, #240*1*2+4]
    ldr     r4, [r1, #240*2*2+4]
    ldr     r5, [r1, #240*3*2+4]
    rb_line_low
    stmia   r0, {r7,r8}
    sub     r0, r0, #240*2
    rb_line_hi
    stmia   r0, {r7,r8}
    sub     r0, r0, #240*2

    subs    lr, lr, #1
    add     r1, r1, #8
    bne     rotated_blit_loop

    ldmfd   sp!,{r4-r8,pc}
*/
	
@ vim:filetype=armasm

