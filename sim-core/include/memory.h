#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

// Byte-addressable, word-aligned flat memory.
// Default size: 4 KB (1024 words). Grows if needed.
// Only word-aligned 4-byte loads/stores are permitted.
class Memory {
public:
    static constexpr uint32_t DEFAULT_SIZE_BYTES = 4096; // 4 KB

    explicit Memory(uint32_t size_bytes = DEFAULT_SIZE_BYTES);

    // Load 32-bit word from byte address (must be 4-byte aligned).
    uint32_t load_word(uint32_t byte_addr) const;

    // Store 32-bit word to byte address (must be 4-byte aligned).
    void store_word(uint32_t byte_addr, uint32_t value);

    // Write a 32-bit instruction word into the instruction region (offset = word_index * 4).
    void write_instruction(uint32_t word_index, uint32_t word);

    // Byte-level write (used by assembler to load the program).
    void write_byte(uint32_t byte_addr, uint8_t value);

    // Reset all memory to zero.
    void reset();

    uint32_t size_bytes() const { return static_cast<uint32_t>(data_.size()); }

private:
    std::vector<uint8_t> data_;

    void check_alignment(uint32_t byte_addr) const;
    void check_bounds(uint32_t byte_addr, uint32_t len) const;
};
