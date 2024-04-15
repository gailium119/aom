/*
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved.
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_RATECTRL_RTC_H_
#define AOM_AV1_RATECTRL_RTC_H_
#if defined(__cplusplus)
#include <cstdint>
#include <memory>
#else
#include <stddef.h>
#include <stdint.h>
#if __STDC__VERSION__ >= 199901L
#include <stdbool.h>
#endif
#endif

#if defined(__cplusplus)
struct AV1_COMP;

namespace aom {

// These constants come from AV1 spec.
static constexpr size_t kAV1MaxLayers = 32;
static constexpr size_t kAV1MaxTemporalLayers = 8;
static constexpr size_t kAV1MaxSpatialLayers = 4;

enum FrameType { kKeyFrame, kInterFrame };

struct AV1RateControlRtcConfig {
 public:
  AV1RateControlRtcConfig();

  int width;
  int height;
  // Flag indicating if the content is screen or not.
  bool is_screen = false;
  // 0-63
  int max_quantizer;
  int min_quantizer;
  int64_t target_bandwidth;
  int64_t buf_initial_sz;
  int64_t buf_optimal_sz;
  int64_t buf_sz;
  int undershoot_pct;
  int overshoot_pct;
  int max_intra_bitrate_pct;
  int max_inter_bitrate_pct;
  int frame_drop_thresh;
  int max_consec_drop;
  double framerate;
  int layer_target_bitrate[kAV1MaxLayers];
  int ts_rate_decimator[kAV1MaxTemporalLayers];
  int aq_mode;
  // Number of spatial layers
  int ss_number_layers;
  // Number of temporal layers
  int ts_number_layers;
  int max_quantizers[kAV1MaxLayers];
  int min_quantizers[kAV1MaxLayers];
  int scaling_factor_num[kAV1MaxSpatialLayers];
  int scaling_factor_den[kAV1MaxSpatialLayers];
};

struct AV1FrameParamsRTC {
  FrameType frame_type;
  int spatial_layer_id;
  int temporal_layer_id;
};

struct AV1LoopfilterLevel {
  int filter_level[2];
  int filter_level_u;
  int filter_level_v;
};

struct AV1CdefInfo {
  int cdef_strength_y;
  int cdef_strength_uv;
  int damping;
};

struct AV1SegmentationData {
  const uint8_t *segmentation_map;
  size_t segmentation_map_size;
  const int *delta_q;
  size_t delta_q_size;
};

enum class FrameDropDecision {
  kOk,    // Frame is encoded.
  kDrop,  // Frame is dropped.
};

class AV1RateControlRTC {
 public:
  static std::unique_ptr<AV1RateControlRTC> Create(
      const AV1RateControlRtcConfig &cfg);
  ~AV1RateControlRTC();

  bool UpdateRateControl(const AV1RateControlRtcConfig &rc_cfg);
  // GetQP() needs to be called after ComputeQP() to get the latest QP
  int GetQP() const;
  // GetLoopfilterLevel() needs to be called after ComputeQP()
  AV1LoopfilterLevel GetLoopfilterLevel() const;
  // GetCdefInfo() needs to be called after ComputeQP()
  AV1CdefInfo GetCdefInfo() const;
  // Returns the segmentation map used for cyclic refresh, based on 4x4 blocks.
  bool GetSegmentationData(AV1SegmentationData *segmentation_data) const;
  // ComputeQP returns the QP if the frame is not dropped (kOk return),
  // otherwise it returns kDrop and subsequent GetQP and PostEncodeUpdate
  // are not to be called (av1_rc_postencode_update_drop_frame is already
  // called via ComputeQP if drop is decided).
  FrameDropDecision ComputeQP(const AV1FrameParamsRTC &frame_params);
  // Feedback to rate control with the size of current encoded frame
  void PostEncodeUpdate(uint64_t encoded_frame_size);

 private:
  AV1RateControlRTC() = default;
  bool InitRateControl(const AV1RateControlRtcConfig &cfg);
  AV1_COMP *cpi_;
  int initial_width_;
  int initial_height_;
};
}  // namespace aom

typedef aom::AV1RateControlRtcConfig AV1RateControlRtcConfigAlias;
typedef aom::AV1LoopfilterLevel AV1LoopfilterLevelAlias;
typedef aom::FrameDropDecision FrameDropDecisionAlias;
typedef aom::AV1FrameParamsRTC AV1FrameParamsRTCAlias;
typedef aom::AV1SegmentationData AV1SegmentationDataAlias;
typedef aom::AV1CdefInfo AV1CdefInfoAlias;
typedef aom::AV1RateControlRtcConfig AV1RateControlRtcConfigAlias;

extern "C" {

void *create_av1_ratecontrol_rtc(const AV1RateControlRtcConfigAlias &rc_cfg);
bool update_ratecontrol_av1(void *controller,
                            const AV1RateControlRtcConfigAlias &rc_cfg);
int get_qp_ratecontrol_av1(void *controller);

AV1LoopfilterLevelAlias get_loop_filter_level_ratecontrol_av1(void *controller);
FrameDropDecisionAlias compute_qp_ratecontrol_av1(
    void *controller, const AV1FrameParamsRTCAlias &frame_params);

void post_encode_update_ratecontrol_av1(void *controller,
                                        uint64_t encoded_frame_size);

bool get_segmentation_data_av1(void *controller,
                               AV1SegmentationDataAlias *segmentation_data);

AV1CdefInfoAlias get_cdef_info_av1(void *controller);

AV1RateControlRtcConfigAlias *create_av1_ratecontrol_config();

void destroy_av1_ratecontrol_rtc(void *controller);
}  // extern "C"

#else  // #if defined(__cplusplus)

/* These constants come from AV1 spec.*/
#define kAV1MaxLayers 32
#define kAV1MaxTemporalLayers 8
#define kAV1MaxSpatialLayers 4

enum FrameType { kKeyFrame = 0, kInterFrame = 1 };

typedef enum { false = 0, true = 1 } bool;

struct AV1RateControlRtcConfig {
  int width;
  int height;
  // Flag indicating if the content is screen or not.
  bool is_screen;
  // 0-63
  int max_quantizer;
  int min_quantizer;
  int64_t target_bandwidth;
  int64_t buf_initial_sz;
  int64_t buf_optimal_sz;
  int64_t buf_sz;
  int undershoot_pct;
  int overshoot_pct;
  int max_intra_bitrate_pct;
  int max_inter_bitrate_pct;
  int frame_drop_thresh;
  int max_consec_drop;
  double framerate;
  int layer_target_bitrate[kAV1MaxLayers];
  int ts_rate_decimator[kAV1MaxTemporalLayers];
  int aq_mode;
  // Number of spatial layers
  int ss_number_layers;
  // Number of temporal layers
  int ts_number_layers;
  int max_quantizers[kAV1MaxLayers];
  int min_quantizers[kAV1MaxLayers];
  int scaling_factor_num[kAV1MaxSpatialLayers];
  int scaling_factor_den[kAV1MaxSpatialLayers];
};

struct AV1FrameParamsRTC {
  // FrameType frame_type;
  int spatial_layer_id;
  int temporal_layer_id;
};

struct AV1LoopfilterLevel {
  int filter_level[2];
  int filter_level_u;
  int filter_level_v;
};

struct AV1CdefInfo {
  int cdef_strength_y;
  int cdef_strength_uv;
  int damping;
};

struct AV1SegmentationData {
  const uint8_t *segmentation_map;
  size_t segmentation_map_size;
  const int *delta_q;
  size_t delta_q_size;
};

enum FrameDropDecision {
  Frame_drop_decision_kOk,    // Frame is encoded.
  Frame_drop_decision_kDrop,  // Frame is dropped.
};

void *create_av1_ratecontrol_rtc(const struct AV1RateControlRtcConfig *rc_cfg);
int update_ratecontrol_av1(void *controller,
                           const struct AV1RateControlRtcConfig *rc_cfg);
int get_qp_ratecontrol_av1(void *controller);

struct AV1LoopfilterLevel get_loop_filter_level_ratecontrol_av1(
    void *controller);
enum FrameDropDecision compute_qp_ratecontrol_av1(
    void *controller, const struct AV1FrameParamsRTC *frame_params);

void post_encode_update_ratecontrol_av1(void *controller,
                                        uint64_t encoded_frame_size);

int get_segmentation_data_av1(void *controller,
                              struct AV1SegmentationData *segmentation_data);

struct AV1CdefInfo get_cdef_info_av1(void *controller);

struct AV1RateControlRtcConfig *create_av1_ratecontrol_config();

void destroy_av1_ratecontrol_rtc(void *controller);

#endif  // __cplusplus
#endif  // AOM_AV1_RATECTRL_RTC_H_
