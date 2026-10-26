# bubble_sort.asm
# Bubble-sort an array of 8 integers in memory.
#
# Memory layout (starting at byte address 0x100 = 256):
#   addr 256: array[0] = 64
#   addr 260: array[1] = 34
#   addr 264: array[2] = 25
#   addr 268: array[3] = 12
#   addr 272: array[4] = 22
#   addr 276: array[5] = 11
#   addr 280: array[6] = 90
#   addr 284: array[7] =  1
#
# Registers:
#   $r1 = base address of array (256)
#   $r2 = outer loop counter i (8 → 1)
#   $r3 = inner loop counter j
#   $r4 = current element a[j]
#   $r5 = next element a[j+1]
#   $r6 = byte offset for a[j]
#   $r7 = temp for swap
#   $r8 = inner loop limit (n - i)
#   $r9 = constant 4 (sizeof word)

main:
    # Initialize array in memory
    ADDI $r1, $r0, 256      # base = 256

    ADDI $r10, $r0, 64
    SW   $r10, $r1, 0
    ADDI $r10, $r0, 34
    SW   $r10, $r1, 4
    ADDI $r10, $r0, 25
    SW   $r10, $r1, 8
    ADDI $r10, $r0, 12
    SW   $r10, $r1, 12
    ADDI $r10, $r0, 22
    SW   $r10, $r1, 16
    ADDI $r10, $r0, 11
    SW   $r10, $r1, 20
    ADDI $r10, $r0, 90
    SW   $r10, $r1, 24
    ADDI $r10, $r0, 1
    SW   $r10, $r1, 28

    # Bubble sort: n=8 passes
    ADDI $r2, $r0, 7        # outer: n-1 passes
    ADDI $r9, $r0, 4        # step = 4 bytes

outer_loop:
    BEQ $r2, $r0, done      # if i == 0, done
    ADDI $r3, $r0, 0        # j = 0
    ADDI $r8, $r2, 0        # inner limit = i (n-1-i comparisons)

inner_loop:
    BEQ $r3, $r8, outer_next  # if j == limit, next outer
    # Load a[j] and a[j+1]
    # byte offset for a[j] = j * 4
    # We use $r6 as byte offset computed by shifting/adding
    # $r6 = j * 4  (use SLL)
    SLL $r6, $r3, 2           # $r6 = j << 2 = j*4
    ADD $r6, $r6, $r1         # $r6 = base + j*4
    LW  $r4, $r6, 0           # $r4 = a[j]
    LW  $r5, $r6, 4           # $r5 = a[j+1]
    # if a[j] <= a[j+1], no swap
    BGE $r5, $r4, no_swap     # if a[j+1] >= a[j], skip
    # Swap
    SW  $r5, $r6, 0           # a[j]   = a[j+1]
    SW  $r4, $r6, 4           # a[j+1] = a[j]
no_swap:
    ADDI $r3, $r3, 1          # j++
    J inner_loop

outer_next:
    ADDI $r2, $r2, -1         # i--
    J outer_loop

done:
    HALT
# Expected: memory at 256..284 = [1, 11, 12, 22, 25, 34, 64, 90]
