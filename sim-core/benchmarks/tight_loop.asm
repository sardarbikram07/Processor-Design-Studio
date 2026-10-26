# tight_loop.asm
# Tight loop: no data dependencies, pure control flow stress test.
# Runs N=100 iterations.
# $r1 = counter (100)

main:
    ADDI $r1, $r0, 100      # counter = 100

loop:
    BEQ  $r1, $r0, done     # if counter == 0, exit
    ADDI $r1, $r1, -1       # counter--
    J    loop

done:
    HALT
# Expected: $r1 = 0
