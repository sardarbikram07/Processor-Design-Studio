# factorial.asm
# Computes n! iteratively.
# Input:  $r1 = n (loaded as immediate)
# Output: $r2 = n!
# Registers: $r0=0, $r1=counter, $r2=result, $r3=temp

main:
    ADDI $r1, $r0, 7        # n = 7  (7! = 5040)
    ADDI $r2, $r0, 1        # result = 1

loop:
    BEQ $r1, $r0, done      # if n == 0, done
    # result = result * n  (multiply via repeated addition)
    # Use $r3 as inner counter, $r4 as accumulator
    ADDI $r4, $r0, 0        # acc = 0
    ADDI $r3, $r1, 0        # inner_count = n
mul_loop:
    BEQ $r3, $r0, mul_done  # if inner_count == 0, done
    ADD $r4, $r4, $r2       # acc += result
    ADDI $r3, $r3, -1       # inner_count--
    J mul_loop
mul_done:
    ADDI $r2, $r4, 0        # result = acc  (copy via ADDI with $r0)
    ADDI $r1, $r1, -1       # n--
    J loop

done:
    HALT
# Expected: $r2 = 5040 (0x13B0) for n=7
