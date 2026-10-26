# hazard_heavy.asm
# Hazard-heavy benchmark to measure the effect of forwarding/stalling on CPI.
# Creates back-to-back RAW hazards (register written in one instruction,
# read immediately in the next). No branches — pure data hazard stress test.
#
# Computes: $r5 = ($r1 + $r2) * 4 + ($r3 + $r4) * 2  (via shifts)
# Expected: $r5 = (1+2)*4 + (3+4)*2 = 12 + 14 = 26

main:
    ADDI $r1, $r0, 1        # r1 = 1
    ADDI $r2, $r0, 2        # r2 = 2
    ADDI $r3, $r0, 3        # r3 = 3
    ADDI $r4, $r0, 4        # r4 = 4

    # RAW chain 1: immediate use of result
    ADD  $r5, $r1, $r2      # r5 = r1 + r2 = 3           [hazard: reads r1,r2]
    SLL  $r5, $r5, 2        # r5 = r5 << 2 = 12           [hazard: reads r5]
    ADD  $r6, $r3, $r4      # r6 = r3 + r4 = 7
    SLL  $r6, $r6, 1        # r6 = r6 << 1 = 14           [hazard: reads r6]
    ADD  $r5, $r5, $r6      # r5 = 12 + 14 = 26           [hazard: reads r5,r6]

    # More RAW chains
    ADDI $r7, $r5, 0        # r7 = r5 = 26               [hazard: reads r5]
    ADD  $r8, $r7, $r5      # r8 = r7 + r5 = 52          [hazard: reads r7,r5]
    SUB  $r9, $r8, $r5      # r9 = r8 - r5 = 26          [hazard: reads r8,r5]
    ADD  $r10, $r9, $r7     # r10 = r9 + r7 = 52         [hazard: reads r9,r7]

    HALT
# Expected: $r5 = 26
