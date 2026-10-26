# matrix_multiply.asm
# Computes C = A * B for 3×3 matrices of 32-bit integers.
# Memory layout (all at base = 256 * 4 = 1024):
#   A: words [256..264]  (9 words, row-major)
#   B: words [265..273]  (9 words, row-major)
#   C: words [274..282]  (9 words, result, row-major)
#
# $r14 = base address (1024)
# $r1  = row i  (0..2)
# $r2  = col j  (0..2)
# $r3  = inner k (0..2)
# $r4  = A[i][k]
# $r5  = B[k][j]
# $r6  = accumulator for C[i][j]
# $r7  = scratch / addresses
# $r8  = loop bounds (3)
# $r9  = A base offset (0)
# $r10 = B base offset (36)   # 9 words * 4 bytes
# $r11 = C base offset (72)   # 18 words * 4 bytes

main:
    ADDI $r14, $r0, 1024    # base address
    ADDI $r8, $r0, 3        # N = 3
    ADDI $r9, $r0, 0        # A offset = 0 bytes from base
    ADDI $r10, $r0, 36      # B offset = 36 bytes from base (9 words * 4)
    ADDI $r11, $r0, 72      # C offset = 72 bytes from base (18 words * 4)

    # ── Initialise A (identity matrix) ────────────────────────────
    # A[0][0]=1, A[1][1]=1, A[2][2]=1, rest 0
    ADDI $r7, $r0, 1
    SW   $r7, $r14, 0       # A[0][0] = 1   (offset 0)
    SW   $r0, $r14, 4       # A[0][1] = 0
    SW   $r0, $r14, 8       # A[0][2] = 0
    SW   $r0, $r14, 12      # A[1][0] = 0
    SW   $r7, $r14, 16      # A[1][1] = 1   (offset 16)
    SW   $r0, $r14, 20      # A[1][2] = 0
    SW   $r0, $r14, 24      # A[2][0] = 0
    SW   $r0, $r14, 28      # A[2][1] = 0
    SW   $r7, $r14, 32      # A[2][2] = 1   (offset 32)

    # ── Initialise B (1..9 row-major) ─────────────────────────────
    ADDI $r7, $r0, 1
    SW   $r7, $r14, 36      # B[0][0] = 1
    ADDI $r7, $r0, 2
    SW   $r7, $r14, 40      # B[0][1] = 2
    ADDI $r7, $r0, 3
    SW   $r7, $r14, 44      # B[0][2] = 3
    ADDI $r7, $r0, 4
    SW   $r7, $r14, 48      # B[1][0] = 4
    ADDI $r7, $r0, 5
    SW   $r7, $r14, 52      # B[1][1] = 5
    ADDI $r7, $r0, 6
    SW   $r7, $r14, 56      # B[1][2] = 6
    ADDI $r7, $r0, 7
    SW   $r7, $r14, 60      # B[2][0] = 7
    ADDI $r7, $r0, 8
    SW   $r7, $r14, 64      # B[2][1] = 8
    ADDI $r7, $r0, 9
    SW   $r7, $r14, 68      # B[2][2] = 9

    # ── Triple loop: for i in 0..2, for j in 0..2, for k in 0..2 ─
    ADDI $r1, $r0, 0        # i = 0
loop_i:
    BGE  $r1, $r8, done     # if i >= 3 goto done
    ADDI $r2, $r0, 0        # j = 0
loop_j:
    BGE  $r2, $r8, next_i   # if j >= 3 goto next_i
    ADDI $r6, $r0, 0        # acc = 0
    ADDI $r3, $r0, 0        # k = 0
loop_k:
    BGE  $r3, $r8, store_c  # if k >= 3 goto store_c

    # addr_A = base + 0 + (i*3 + k)*4
    # compute i*3: use r7 as temp
    ADDI $r7, $r0, 3
    AND  $r4, $r1, $r1      # $r4 = i (copy)  — use ADD $r4,$r1,$r0
    ADD  $r4, $r1, $r0      # $r4 = i
    # multiply i*3 using repeated add: i*3 = i+i+i
    ADD  $r4, $r4, $r1      # $r4 = 2i
    ADD  $r4, $r4, $r1      # $r4 = 3i
    ADD  $r4, $r4, $r3      # $r4 = 3i + k
    # byte offset: *4 → shift left 2
    SLL  $r4, $r4, 2        # $r4 = (3i+k)*4 bytes
    ADD  $r7, $r14, $r4     # effective address for A
    LW   $r4, $r7, 0        # $r4 = A[i][k]

    # addr_B = base + 36 + (k*3 + j)*4
    ADD  $r5, $r3, $r0      # $r5 = k
    ADD  $r5, $r5, $r3      # $r5 = 2k
    ADD  $r5, $r5, $r3      # $r5 = 3k
    ADD  $r5, $r5, $r2      # $r5 = 3k + j
    SLL  $r5, $r5, 2        # $r5 = (3k+j)*4 bytes
    ADDI $r5, $r5, 36       # add B's start offset
    ADD  $r7, $r14, $r5     # effective address for B
    LW   $r5, $r7, 0        # $r5 = B[k][j]

    # acc += A[i][k] * B[k][j]  — approximate multiply via repeated add
    # For the identity matrix A, only the diagonal k==i term is non-zero,
    # so this loop effectively just copies B[i][j] into C[i][j].
    # Use $r5 as the multiplier count, $r4 as multiplicand
    # Simple: acc += A[i][k] * B[k][j] using a manual multiply loop
    ADDI $r13, $r0, 0       # product = 0
    ADD  $r12, $r4, $r0     # multiplier copy = A[i][k]
mul_loop:
    BEQ  $r12, $r0, mul_done   # while multiplier != 0
    ADD  $r13, $r13, $r5       # product += B[k][j]
    ADDI $r12, $r12, -1        # multiplier--
    J    mul_loop
mul_done:
    ADD  $r6, $r6, $r13     # acc += product

    ADDI $r3, $r3, 1        # k++
    J    loop_k

store_c:
    # addr_C = base + 72 + (i*3 + j)*4
    ADD  $r7, $r1, $r0      # $r7 = i
    ADD  $r7, $r7, $r1      # $r7 = 2i
    ADD  $r7, $r7, $r1      # $r7 = 3i
    ADD  $r7, $r7, $r2      # $r7 = 3i + j
    SLL  $r7, $r7, 2        # $r7 = (3i+j)*4
    ADDI $r7, $r7, 72       # add C's start offset
    ADD  $r13, $r14, $r7    # effective address for C
    SW   $r6, $r13, 0       # C[i][j] = acc

    ADDI $r2, $r2, 1        # j++
    J    loop_j

next_i:
    ADDI $r1, $r1, 1        # i++
    J    loop_i

done:
    HALT
