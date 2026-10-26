#pragma once
#include "isa.h"
#include <cstdint>

// Decode a 32-bit encoded instruction word into an Instruction struct.
// Throws std::runtime_error on unknown opcode/funct.
Instruction decode(uint32_t word);
