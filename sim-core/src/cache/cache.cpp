#include "cache/cache.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <random>

Cache::Cache(uint32_t size_bytes, uint32_t block_size, uint32_t associativity,
             ReplacementPolicy policy)
    : size_bytes_(size_bytes), block_size_(block_size), assoc_(associativity),
      policy_(policy)
{
    if (block_size == 0 || (block_size & (block_size - 1)) != 0)
        throw std::invalid_argument("Cache block size must be a power of 2");
    if (associativity == 0)
        throw std::invalid_argument("Cache associativity must be >= 1");

    num_sets_ = size_bytes / (block_size * associativity);
    if (num_sets_ == 0) num_sets_ = 1;

    sets_.resize(num_sets_, std::vector<CacheLine>(assoc_));
}

ReplacementPolicy Cache::parse_policy(const std::string& s) {
    if (s == "LRU")    return ReplacementPolicy::LRU;
    if (s == "FIFO")   return ReplacementPolicy::FIFO;
    if (s == "Random" || s == "RANDOM") return ReplacementPolicy::RANDOM;
    return ReplacementPolicy::LRU; // default
}

uint32_t Cache::set_index(uint32_t addr) const {
    return (addr / block_size_) % num_sets_;
}

uint64_t Cache::tag_of(uint32_t addr) const {
    return static_cast<uint64_t>(addr / block_size_) / num_sets_;
}

int Cache::find_way(uint32_t set_idx, uint64_t tag) const {
    const auto& set = sets_[set_idx];
    for (uint32_t w = 0; w < assoc_; ++w)
        if (set[w].valid && set[w].tag == tag) return static_cast<int>(w);
    return -1;
}

int Cache::choose_victim(uint32_t set_idx) const {
    const auto& set = sets_[set_idx];

    // First: prefer an invalid (empty) way
    for (uint32_t w = 0; w < assoc_; ++w)
        if (!set[w].valid) return static_cast<int>(w);

    // All ways valid: use replacement policy
    switch (policy_) {
        case ReplacementPolicy::LRU: {
            int lru_way = 0;
            uint32_t oldest = set[0].lru_time;
            for (uint32_t w = 1; w < assoc_; ++w)
                if (set[w].lru_time < oldest) { oldest = set[w].lru_time; lru_way = static_cast<int>(w); }
            return lru_way;
        }
        case ReplacementPolicy::FIFO: {
            int fifo_way = 0;
            uint32_t oldest = set[0].fifo_order;
            for (uint32_t w = 1; w < assoc_; ++w)
                if (set[w].fifo_order < oldest) { oldest = set[w].fifo_order; fifo_way = static_cast<int>(w); }
            return fifo_way;
        }
        case ReplacementPolicy::RANDOM:
        default: {
            // Simple deterministic "random" using a linear congruential generator
            static std::mt19937 rng(42);
            std::uniform_int_distribution<uint32_t> dist(0, assoc_ - 1);
            return static_cast<int>(dist(rng));
        }
    }
}

void Cache::install(uint32_t set_idx, uint64_t tag) {
    int victim = choose_victim(set_idx);
    auto& line       = sets_[set_idx][victim];
    line.valid       = true;
    line.tag         = tag;
    line.lru_time    = timer_;
    line.fifo_order  = timer_;  // FIFO: set at install time, never update
    ++timer_;
}

void Cache::update_lru(uint32_t set_idx, int way) {
    sets_[set_idx][way].lru_time = timer_++;
}

bool Cache::access(uint32_t byte_addr) {
    uint32_t si = set_index(byte_addr);
    uint64_t t  = tag_of(byte_addr);
    int way = find_way(si, t);

    if (way >= 0) {
        // Hit
        if (policy_ == ReplacementPolicy::LRU)
            update_lru(si, way);
        stats_.hits++;
        return true;
    } else {
        // Miss: allocate (load from memory)
        install(si, t);
        stats_.misses++;
        return false;
    }
}

bool Cache::store(uint32_t byte_addr) {
    // Write-through, no-allocate:
    //   - If hit: update cache line (no dirty bit needed), write through to memory
    //   - If miss: write directly to memory, do NOT allocate a cache line
    uint32_t si = set_index(byte_addr);
    uint64_t t  = tag_of(byte_addr);
    int way = find_way(si, t);

    if (way >= 0) {
        // Hit: update LRU (data is already in cache, memory updated by caller)
        if (policy_ == ReplacementPolicy::LRU)
            update_lru(si, way);
        stats_.hits++;
        return true;
    } else {
        // Miss: write-through, no-allocate — don't install
        stats_.misses++;
        return false;
    }
}
