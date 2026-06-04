#include "unit_revfx.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x4D616A69U,
    .unit_id = 0x00000001U,
    .version = 0x00010005U,
    .name = "Squall",
    .num_params = 4,

    .params = {
        // Slot 0 (SDK "time") is presented as TONE in this unit.
        // TONE controls damping/brightness in the reverb core.
        {0, 1023, 0, 384, k_unit_param_type_none, 1, 0, 0, {"TONE"}},
        // DEPTH (Knob B) is the active reverb wetness control.
        {0, 1023, 0, 512, k_unit_param_type_none, 1, 0, 0, {"DPTH"}},
        // MIX stays in place for compatibility, but is currently disused.
        {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1, 1, 0, {"MIX"}},
        // FREEZE lives in the next available slot (index 3).
        {0, 1023, 0, 0, k_unit_param_type_none, 1, 0, 0, {"FREEZE"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
};
