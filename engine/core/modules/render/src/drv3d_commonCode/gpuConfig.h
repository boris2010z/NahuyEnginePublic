// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include "nau/3d/dag_gpuConfig.h"

struct GpuDriverConfig : GpuUserConfig
{
  GpuDriverConfig();

  bool forceFullscreenToWindowed;
  bool flushBeforeSurvey;
};

const GpuDriverConfig &get_gpu_driver_cfg();