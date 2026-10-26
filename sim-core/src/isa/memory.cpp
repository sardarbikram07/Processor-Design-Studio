#include "memory.h"
#include <stdexcept>
#include <cstring>

Memory::Memory(uint32_t size_bytes) : data_(size_bytes, 0) {}

void Memory::check_alignment(uint32_t byte_addr) const {
    if (byte_addr % 4 != 0)
        throw std::runtime_error("Unaligned memory access at 0x" +
                                 std::to_string(byte_addr));
}

void Memory::check_bounds(uint32_t byte_addr, uint32_t len) const {
    if (static_cast<uint64_t>(byte_addr) + len > data_.size())
        throw std::runtime_error("Memory access out of bounds at 0x" +
                                 std::to_string(byte_addr));
}

uint32_t Memory::load_word(uint32_t byte_addr) const {
    check_alignment(byte_addr);
    check_bounds(byte_addr, 4);
    uint32_t word = 0;
    // Big-endian field layout as specified
    word |= static_cast<uint32_t>(data_[byte_addr + 0]) << 24;
    word |= static_cast<uint32_t>(data_[byte_addr + 1]) << 16;
    word |= static_cast<uint32_t>(data_[byte_addr + 2]) <<  8;
    word |= static_cast<uint32_t>(data_[byte_addr + 3]) <<  0;
    return word;
}

void Memory::store_word(uint32_t byte_addr, uint32_t value) {
    check_alignment(byte_addr);
    check_bounds(byte_addr, 4);
    data_[byte_addr + 0] = (value >> 24) & 0xFF;
    data_[byte_addr + 1] = (value >> 16) & 0xFF;
    data_[byte_addr + 2] = (value >>  8) & 0xFF;
    data_[byte_addr + 3] = (value >>  0) & 0xFF;
}

void Memory::write_instruction(uint32_t word_index, uint32_t word) {
    store_word(word_index * 4, word);
}

void Memory::write_byte(uint32_t byte_addr, uint8_t value) {
    check_bounds(byte_addr, 1);
    data_[byte_addr] = value;
}

void Memory::reset() {
    std::fill(data_.begin(), data_.end(), 0);
}
