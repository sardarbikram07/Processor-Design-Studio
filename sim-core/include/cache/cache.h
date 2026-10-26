#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

// ─── Cache Statistics ─────────────────────────────────────────────────────────
struct CacheStats {
    uint64_t hits        = 0;
    uint64_t misses      = 0;
    uint64_t accesses()  const { return hits + misses; }
    double   hit_rate()  const {
        return accesses() > 0
            ? static_cast<double>(hits) / static_cast<double>(accesses())
            : 0.0;
    }
};

// ─── Replacement Policy ───────────────────────────────────────────────────────
enum class ReplacementPolicy { LRU, FIFO, RANDOM, Random = RANDOM };

// ─── Cache Line ───────────────────────────────────────────────────────────────
struct CacheLine {
    bool     valid    = false;
    uint64_t tag      = 0;
    uint32_t lru_time = 0;  // LRU timestamp
    uint32_t fifo_order = 0; // FIFO insertion order
};

// ─── Set-Associative Cache ────────────────────────────────────────────────────
// Configurable: size (bytes), block size (bytes), associativity (ways),
// replacement policy (LRU/FIFO/Random).
// Write policy: write-through, no-allocate (as specified).
//
// Geometry:
//   sets      = sizeBytes / (blockSize * associativity)
//   set_index = (byte_addr / blockSize) % sets
//   tag       = byte_addr / (blockSize * sets)
class Cache {
public:
    Cache(uint32_t size_bytes, uint32_t block_size, uint32_t associativity,
          ReplacementPolicy policy);

    // Perform a load access. Returns true on hit, false on miss.
    bool access(uint32_t byte_addr);

    // Perform a store access (write-through, no-allocate).
    // Returns true on hit (dirty write), false on miss (write bypasses cache).
    bool store(uint32_t byte_addr);

    const CacheStats& stats() const { return stats_; }
    void reset_stats() { stats_ = CacheStats{}; }

    static ReplacementPolicy parse_policy(const std::string& s);

private:
    uint32_t         size_bytes_;
    uint32_t         block_size_;
    uint32_t         assoc_;
    uint32_t         num_sets_;
    ReplacementPolicy policy_;
    uint32_t         timer_   = 0; // global timestamp for LRU/FIFO

    // [set_index][way]
    std::vector<std::vector<CacheLine>> sets_;

    CacheStats stats_;

    uint32_t set_index(uint32_t addr) const;
    uint64_t tag_of(uint32_t addr) const;
    int  find_way(uint32_t set_idx, uint64_t tag) const;  // -1 if miss
    int  choose_victim(uint32_t set_idx) const;
    void install(uint32_t set_idx, uint64_t tag);
    void update_lru(uint32_t set_idx, int way);
};
