/**
  Copyright (c) 2023 by Contributors
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

union Entry {
  int missing;
  float fvalue;
  int qvalue;
};

size_t get_num_class(void) {
  return 1;
}

size_t get_num_feature(void) {
  return 127;
}

char const* get_pred_transform(void) {
  return "sigmoid";
}

float get_sigmoid_alpha(void) {
  return 1;
}

float get_ratio_c(void) {
  return 1;
}

float get_global_bias(void) {
  return -0;
}

char const* get_threshold_type(void) {
  return "float32";
}

char const* get_leaf_output_type(void) {
  return "float32";
}

static inline float pred_transform(float margin) {
  float const alpha = (float)1;
  return 1.0f / (1 + expf(-alpha * margin));
}
float predict(union Entry* data, int pred_margin) {
  float sum = 0.0f;
  unsigned int tmp;
  int nid, cond, fid; /* used for folded subtrees */
  if (!(data[29].missing != -1) || (data[29].fvalue < -9.5367431640625e-07)) {
    if (!(data[56].missing != -1) || (data[56].fvalue < -9.5367431640625e-07)) {
      if (!(data[60].missing != -1) || (data[60].fvalue < -9.5367431640625e-07)) {
        sum += (float)1.8989964723587036;
      } else {
        sum += (float)-1.9473683834075928;
      }
    } else {
      if (!(data[21].missing != -1) || (data[21].fvalue < -9.5367431640625e-07)) {
        sum += (float)1.7837837934494019;
      } else {
        sum += (float)-1.9813519716262817;
      }
    }
  } else {
    if (!(data[109].missing != -1) || (data[109].fvalue < -9.5367431640625e-07)) {
      if (!(data[67].missing != -1) || (data[67].fvalue < -9.5367431640625e-07)) {
        sum += (float)-1.9854598045349121;
      } else {
        sum += (float)0.93877553939819336;
      }
    } else {
      sum += (float)1.8709677457809448;
    }
  }
  if (!(data[29].missing != -1) || (data[29].fvalue < -9.5367431640625e-07)) {
    if (!(data[21].missing != -1) || (data[21].fvalue < -9.5367431640625e-07)) {
      sum += (float)1.1460790634155273;
    } else {
      if (!(data[36].missing != -1) || (data[36].fvalue < -9.5367431640625e-07)) {
        sum += (float)-6.8799467086791992;
      } else {
        sum += (float)-0.10659158974885941;
      }
    }
  } else {
    if (!(data[109].missing != -1) || (data[109].fvalue < -9.5367431640625e-07)) {
      if (!(data[39].missing != -1) || (data[39].fvalue < -9.5367431640625e-07)) {
        sum += (float)-0.0930657759308815;
      } else {
        sum += (float)-1.1526120901107788;
      }
    } else {
      sum += (float)1.0042307376861572;
    }
  }

  sum = sum + (float)(-0);
  if (!pred_margin) {
    return pred_transform(sum);
  } else {
    return sum;
  }
}
