#pragma once
#include "stages.h"

// ─── Hazard Detection ─────────────────────────────────────────────────────────
// Detects RAW (Read-After-Write) data hazards and control hazards.
// Returns true if a stall is needed (only when forwarding is disabled or
// load-use hazard which can't be forwarded regardless).

struct HazardInfo {
    bool stall_needed   = false;  // Insert pipeline bubble + freeze PC/IF-ID
    bool flush_needed   = false;  // Branch/jump taken: squash IF and ID stages
};

class HazardDetector {
public:
    // Detect stalls.
    // Called after ID (register read) and before EX.
    // stalling_enabled: from config (if false, data hazards cause incorrect results
    //   unless forwarding handles them — forwarding is checked separately).
    static HazardInfo detect(
        const ID_EX&   id_ex,    // instruction currently in EX stage
        const EX_MEM&  ex_mem,   // instruction currently in MEM stage
        const IF_ID&   if_id,    // instruction currently in ID stage (just decoded)
        uint8_t if_id_rs,        // rs of the instruction in IF/ID
        uint8_t if_id_rt,        // rt of the instruction in IF/ID
        bool stalling_enabled,
        bool forwarding_enabled
    );
};
