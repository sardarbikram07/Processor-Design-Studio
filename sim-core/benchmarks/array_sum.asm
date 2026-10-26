# array_sum.asm
# Sums an array of N integers stored in memory.
# Memory layout: N=8 values starting at address 512 (0x200)
# $r1 = base address (512)
# $r2 = count (8)
# $r3 = sum (result)
# $r4 = loop counter i
# $r5 = temp (current element)

main:
    ADDI $r1, $r0, 512      # base = 512
    ADDI $r2, $r0, 8        # count = 8

    # Initialize array: [10, 20, 30, 40, 50, 60, 70, 80]
    ADDI $r10, $r0, 10
    SW   $r10, $r1, 0
    ADDI $r10, $r0, 20
    SW   $r10, $r1, 4
    ADDI $r10, $r0, 30
    SW   $r10, $r1, 8
    ADDI $r10, $r0, 40
    SW   $r10, $r1, 12
    ADDI $r10, $r0, 50
    SW   $r10, $r1, 16
    ADDI $r10, $r0, 60
    SW   $r10, $r1, 20
    ADDI $r10, $r0, 70
    SW   $r10, $r1, 24
    ADDI $r10, $r0, 80
    SW   $r10, $r1, 28

    ADDI $r3, $r0, 0        # sum = 0
    ADDI $r4, $r0, 0        # i = 0

loop:
    BEQ  $r4, $r2, done     # if i == count, done
    SLL  $r6, $r4, 2        # offset = i * 4
    ADD  $r6, $r6, $r1      # addr = base + offset
    LW   $r5, $r6, 0        # r5 = array[i]
    ADD  $r3, $r3, $r5      # sum += array[i]
    ADDI $r4, $r4, 1        # i++
    J    loop

done:
    HALT
# Expected: $r3 = 10+20+30+40+50+60+70+80 = 360
