#include "predictor/predictor.h"
#include <stdexcept>

PredictorType BranchPredictor::parse(const std::string& s) {
    if (s == "STATIC_NOT_TAKEN") return PredictorType::STATIC_NOT_TAKEN;
    if (s == "STATIC_TAKEN")     return PredictorType::STATIC_TAKEN;
    if (s == "ONE_BIT")          return PredictorType::ONE_BIT;
    if (s == "TWO_BIT_SATURATING") return PredictorType::TWO_BIT_SATURATING;
    return PredictorType::STATIC_NOT_TAKEN; // default
}

BranchPredictor* BranchPredictor::create(const std::string& type_str) {
    switch (parse(type_str)) {
        case PredictorType::STATIC_NOT_TAKEN:  return new StaticNotTaken();
        case PredictorType::STATIC_TAKEN:      return new StaticTaken();
        case PredictorType::ONE_BIT:           return new OneBitPredictor();
        case PredictorType::TWO_BIT_SATURATING: return new TwoBitSaturating();
        default: return new StaticNotTaken();
    }
}
