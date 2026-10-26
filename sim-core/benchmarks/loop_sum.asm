# loop_sum.asm
# Computes sum of 1..N (Gauss sum).
# Input:  $r1 = N (set to 100)
# Output: $r2 = N*(N+1)/2
# This is the same example from the spec.

main:
    ADDI $r1, $r0, 10       # counter = 10
    ADDI $r2, $r0, 0        # sum = 0

loop:
    BEQ $r1, $r0, end       # if counter == 0, exit
    ADD $r2, $r2, $r1       # sum += counter
    ADDI $r1, $r1, -1       # counter--
    J loop

end:
    HALT
# Expected: $r2 = 55  (sum of 1..10)
