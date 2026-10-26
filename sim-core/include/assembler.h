#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Two-pass assembler.
//   Pass 1: scan for labels, build label→word-address table.
//   Pass 2: encode each instruction to a 32-bit word per the ISA encoding.
//
// Returns a vector of 32-bit encoded instruction words (program image).
// Throws std::runtime_error on any syntax or encoding error.
std::vector<uint32_t> assemble(const std::string& source);

// Convenience: assemble from file path.
std::vector<uint32_t> assemble_file(const std::string& path);
