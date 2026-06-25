#include "unit_revfx.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x4D616A69U,
    .unit_id = 0x00000001U,
    // Encoded as 0x00MMmmpp (major.minor.patch).
    .version = 0x00010100U,
    .name = "Squall",
    .num_params = 5,

    .params = {
        // Slot 0 (SDK TIME) is labeled TONE in this unit.
        {0, 1023, 0, 384, k_unit_param_type_none, 1, 0, 0, {"TONE"}},
        // DEPTH is the main reverb amount control.
        {0, 1023, 0, 512, k_unit_param_type_none, 1, 0, 0, {"DPTH"}},
        // MIX stays in the fixed SDK slot for editor compatibility.
        {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1, 1, 0, {"MIX"}},
        // FREEZE lives in slot 3 and defaults OFF.
        {0, 1023, 0, 0, k_unit_param_type_none, 1, 0, 0, {"FREEZE"}},
        // SCAN lives in slot 4 and defaults to 100%.
        // SCAN controls freeze leak and colors frozen texture via LP/diffusion.
        {0, 1023, 0, 1023, k_unit_param_type_none, 1, 0, 0, {"SCAN"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
};
