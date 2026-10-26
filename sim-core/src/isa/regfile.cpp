#include "regfile.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

RegisterFile::RegisterFile() { reset(); }

void RegisterFile::reset() { regs_.fill(0); }

int32_t RegisterFile::read(uint8_t reg) const {
    if (reg >= NUM_REGS)
        throw std::out_of_range("Register index out of range: " + std::to_string(reg));
    return regs_[reg];      // regs_[0] is always 0 because writes are blocked
}

void RegisterFile::write(uint8_t reg, int32_t value) {
    if (reg >= NUM_REGS)
        throw std::out_of_range("Register index out of range: " + std::to_string(reg));
    if (reg == 0) return;   // $r0 hardwired to 0
    regs_[reg] = value;
}

void RegisterFile::dump() const {
    for (int i = 0; i < NUM_REGS; ++i) {
        std::cout << "$r" << std::setw(2) << std::setfill('0') << i
                  << " = " << std::setw(11) << regs_[i]
                  << "  (0x" << std::hex << std::setw(8) << std::setfill('0')
                  << static_cast<uint32_t>(regs_[i]) << std::dec << ")\n";
    }
}
