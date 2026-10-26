#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

// ─── Predictor types (matches spec §4) ────────────────────────────────────────
enum class PredictorType {
    STATIC_NOT_TAKEN,      // always predict not taken
    STATIC_TAKEN,          // always predict taken
    ONE_BIT,               // 1-bit bimodal (last outcome)
    TWO_BIT_SATURATING,    // 2-bit saturating counter (standard)
};

// ─── Branch Predictor Interface ───────────────────────────────────────────────
// All predictors are indexed by the PC of the branch instruction.
class BranchPredictor {
public:
    virtual ~BranchPredictor() = default;

    // Predict whether the branch at `pc` is taken.
    // Returns true → predict taken, false → predict not taken.
    virtual bool predict(uint32_t pc) = 0;

    // Update the predictor after the branch resolves.
    // `taken` = actual outcome at `pc`.
    virtual void update(uint32_t pc, bool taken) = 0;

    // Statistics
    struct Stats {
        uint64_t predictions   = 0;
        uint64_t mispredictions = 0;
        double misprediction_rate() const {
            return predictions > 0
                ? static_cast<double>(mispredictions) / static_cast<double>(predictions)
                : 0.0;
        }
    };
    const Stats& stats() const { return stats_; }
    void reset_stats() { stats_ = Stats{}; }

    // Factory: create predictor from config string
    static BranchPredictor* create(const std::string& type_str);
    static PredictorType parse(const std::string& s);

protected:
    Stats stats_;
};

// ─── Static Not-Taken ─────────────────────────────────────────────────────────
class StaticNotTaken : public BranchPredictor {
public:
    bool predict(uint32_t /*pc*/) override { return false; }
    void update(uint32_t /*pc*/, bool taken) override {
        stats_.predictions++;
        if (taken) stats_.mispredictions++;
    }
};

// ─── Static Taken ─────────────────────────────────────────────────────────────
class StaticTaken : public BranchPredictor {
public:
    bool predict(uint32_t /*pc*/) override { return true; }
    void update(uint32_t /*pc*/, bool taken) override {
        stats_.predictions++;
        if (!taken) stats_.mispredictions++;
    }
};

// ─── 1-Bit Bimodal ────────────────────────────────────────────────────────────
// One bit per branch: stores last outcome.
class OneBitPredictor : public BranchPredictor {
public:
    bool predict(uint32_t pc) override {
        auto it = table_.find(pc);
        return (it != table_.end()) ? it->second : false; // cold = not taken
    }
    void update(uint32_t pc, bool taken) override {
        bool prediction = predict(pc);
        stats_.predictions++;
        if (prediction != taken) stats_.mispredictions++;
        table_[pc] = taken;
    }
private:
    std::unordered_map<uint32_t, bool> table_;
};

// ─── 2-Bit Saturating Counter ─────────────────────────────────────────────────
// States: 0=StrongNT, 1=WeakNT, 2=WeakT, 3=StrongT
// Cold entry = 1 (WeakNT = predict not-taken)
class TwoBitSaturating : public BranchPredictor {
public:
    bool predict(uint32_t pc) override {
        auto it = table_.find(pc);
        uint8_t state = (it != table_.end()) ? it->second : 1; // cold = WeakNT
        return state >= 2; // taken if WeakT or StrongT
    }
    void update(uint32_t pc, bool taken) override {
        bool prediction = predict(pc);
        stats_.predictions++;
        if (prediction != taken) stats_.mispredictions++;

        auto it = table_.find(pc);
        uint8_t state = (it != table_.end()) ? it->second : 1;
        if (taken) {
            if (state < 3) state++;
        } else {
            if (state > 0) state--;
        }
        table_[pc] = state;
    }
private:
    std::unordered_map<uint32_t, uint8_t> table_;
};
