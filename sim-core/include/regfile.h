#pragma once
#include <cstdint>
#include <array>
#include <stdexcept>

// 16-register file: $r0–$r15
// $r0 is hardwired to 0; writes to $r0 are silently ignored.
class RegisterFile {
public:
    RegisterFile();

    // Read register value. $r0 always returns 0.
    int32_t read(uint8_t reg) const;

    // Write register value. Writes to $r0 are ignored.
    void write(uint8_t reg, int32_t value);

    // Reset all registers to 0.
    void reset();

    // Debug: dump all register values.
    void dump() const;

    static constexpr int NUM_REGS = 16;

private:
    std::array<int32_t, NUM_REGS> regs_;
};
