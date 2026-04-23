#pragma once

/* E5's legacy cuda→hip shim; now superseded by the repo-wide
 * ../common/gpu_compat.cuh. Kept as a forwarder so existing
 * `#include "gpu_runtime.h"` sites still work. */
#include "../common/gpu_compat.cuh"
