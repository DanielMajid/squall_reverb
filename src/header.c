#include "unit_revfx.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x4D616A69U,
    .unit_id = 0x00000001U,
    .version = 0x00010120U,
    .name = "Squall",
    .num_params = 5,

    .params = {
        // Slot 0 (SDK TIME) is labeled TONE in this unit.
        {0, 1023, 0, 384, k_unit_param_type_none, 1, 0, 0, {"TONE"}},
        // DEPTH is the main reverb amount control.
        {0, 1023, 0, 512, k_unit_param_type_none, 1, 0, 0, {"DPTH"}},
        // MIX stays in the fixed SDK slot for editor compatibility.
        {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1, 1, 0, {"MIX"}},
        // FREEZE lives in slot 3.
        {0, 1023, 0, 0, k_unit_param_type_none, 1, 0, 0, {"FREEZE"}},
        // FTMBR in slot 4 sets how loose or locked freeze feels.
        {0, 1023, 0, 256, k_unit_param_type_none, 1, 0, 0, {"FTMBR"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
};
