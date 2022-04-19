/*
 * Copyright (c) 2022, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "av1/reference_manager.h"
#include <algorithm>
#include <vector>

namespace aom {

void RefFrameManager::Reset() {
  free_ref_idx_list_.clear();
  for (int i = 0; i < kRefFrameTableSize; ++i) {
    free_ref_idx_list_.push_back(i);
  }
  forward_stack_.clear();
  backward_queue_.clear();
  last_queue_.clear();
}

int RefFrameManager::AllocateRefIdx() {
  if (free_ref_idx_list_.empty()) {
    size_t backward_size = backward_queue_.size();
    size_t last_size = last_queue_.size();
    if (last_size >= backward_size) {
      int ref_idx = last_queue_.front();
      last_queue_.pop_front();
      free_ref_idx_list_.push_back(ref_idx);
    } else {
      int ref_idx = backward_queue_.front();
      backward_queue_.pop_front();
      free_ref_idx_list_.push_back(ref_idx);
    }
  }

  int ref_idx = free_ref_idx_list_.front();
  free_ref_idx_list_.pop_front();
  return ref_idx;
}

int RefFrameManager::GetExistRefFrameCount() const {
  size_t cnt =
      forward_stack_.size() + backward_queue_.size() + last_queue_.size();
  return static_cast<int>(cnt);
}

// TODO(angiebird): Add unit test.
// Find the ref_idx corresponding to a ref_update_type.
// Return -1 if no ref frame is found.
// The priority_idx indicate closeness between the current frame and
// the ref frame in display order.
// For example, ref_update_type == kForward and priority_idx == 0 means
// find the closest ref frame in forward_stack_.
int RefFrameManager::GetRefFrameIdx(RefUpdateType ref_update_type,
                                    int priority_idx) const {
  if (ref_update_type == RefUpdateType::kForward) {
    int size = static_cast<int>(forward_stack_.size());
    if (priority_idx < size) {
      return forward_stack_[size - priority_idx - 1];
    }
  } else if (ref_update_type == RefUpdateType::kBackward) {
    int size = static_cast<int>(backward_queue_.size());
    if (priority_idx < size) {
      return backward_queue_[size - priority_idx - 1];
    }
  } else if (ref_update_type == RefUpdateType::kLast) {
    int size = static_cast<int>(last_queue_.size());
    if (priority_idx < size) {
      return last_queue_[size - priority_idx - 1];
    }
  }
  return -1;
}

// TODO(angiebird):
// 1) Add unit test
// 2) Make sure the name lists are long enough.
// 3) This function should be further optimized based on encoder behavior.
//
// Assign ref name based on ref_update_type on ref_update_type and priority_idx.
ReferenceName get_ref_name(RefUpdateType ref_update_type, int priority_idx,
                           const std::vector<ReferenceName> &used_name_list) {
  const std::vector<ReferenceName> forward_name_list{
    ReferenceName::kBwdrefFrame, ReferenceName::kAltref2Frame,
    ReferenceName::kAltrefFrame
  };
  const std::vector<ReferenceName> backward_name_list{
    ReferenceName::kGoldenFrame, ReferenceName::kLast2Frame,
    ReferenceName::kLast3Frame
  };
  const std::vector<ReferenceName> last_name_list{ ReferenceName::kLastFrame,
                                                   ReferenceName::kLast2Frame,
                                                   ReferenceName::kLast3Frame };
  const std::vector<ReferenceName> *name_list;
  if (ref_update_type == RefUpdateType::kForward) {
    name_list = &forward_name_list;
  }
  if (ref_update_type == RefUpdateType::kBackward) {
    name_list = &backward_name_list;
  }
  if (ref_update_type == RefUpdateType::kLast) {
    name_list = &last_name_list;
  }

  const int name_list_size = static_cast<int>(name_list->size());
  for (int idx = priority_idx; idx < name_list_size; idx++) {
    ReferenceName ref_name = name_list->at(idx);
    bool not_used = std::find(used_name_list.begin(), used_name_list.end(),
                              ref_name) == used_name_list.end();
    if (not_used) return ref_name;
  }
  return ReferenceName::kNoneFrame;
}

std::vector<ReferenceFrame> RefFrameManager::GetRefFrameList() const {
  std::vector<ReferenceFrame> ref_frame_list;
  constexpr int round_robin_size = 3;
  const std::vector<RefUpdateType> round_robin_list{ RefUpdateType::kForward,
                                                     RefUpdateType::kBackward,
                                                     RefUpdateType::kLast };
  std::vector<int> priority_idx_list(round_robin_size, 0);
  const int exist_ref_frame_count = GetExistRefFrameCount();
  int ref_frame_count = static_cast<int>(ref_frame_list.size());
  int round_robin_idx = 0;
  std::vector<ReferenceName> used_name_list;
  while (ref_frame_count < max_ref_frames_ ||
         ref_frame_count < exist_ref_frame_count) {
    const auto ref_update_type = round_robin_list[round_robin_idx];
    auto &priority_idx = priority_idx_list[round_robin_idx];
    int ref_idx = GetRefFrameIdx(ref_update_type, priority_idx);
    if (ref_idx != -1) {
      const auto name =
          get_ref_name(ref_update_type, priority_idx, used_name_list);
      // TODO(angiebird): Make sure we always get a valid name here
      assert(name != ReferenceName::kNoneFrame);
      used_name_list.push_back(name);
      ReferenceFrame ref_frame = { ref_idx, name };
      ref_frame_list.push_back(ref_frame);
      priority_idx++;
    }
    ref_frame_count = static_cast<int>(ref_frame_list.size());
    round_robin_idx = (round_robin_idx + 1) % round_robin_size;
  }
  return ref_frame_list;
}

void RefFrameManager::UpdateOrder(int global_order_idx) {
  if (forward_stack_.empty()) {
    return;
  }
  int ref_idx = forward_stack_.back();
  const GopFrame &gf_frame = ref_frame_table_[ref_idx];
  if (gf_frame.global_order_idx <= global_order_idx) {
    forward_stack_.pop_back();
    if (gf_frame.is_golden_frame) {
      // high quality frame
      backward_queue_.push_back(ref_idx);
    } else {
      last_queue_.push_back(ref_idx);
    }
  }
}

int RefFrameManager::ColocatedRefIdx(int global_order_idx) {
  if (forward_stack_.size() == 0) return -1;
  int ref_idx = forward_stack_.back();
  int arf_global_order_idx = ref_frame_table_[ref_idx].global_order_idx;
  if (arf_global_order_idx == global_order_idx) {
    return ref_idx;
  }
  return -1;
}

void RefFrameManager::UpdateRefFrameTable(GopFrame *gop_frame,
                                          RefUpdateType ref_update_type,
                                          EncodeRefMode encode_ref_mode) {
  gop_frame->encode_ref_mode = encode_ref_mode;
  gop_frame->ref_frame_list = GetRefFrameList();
  gop_frame->colocated_ref_idx = ColocatedRefIdx(gop_frame->global_order_idx);
  if (gop_frame->is_show_frame) {
    UpdateOrder(gop_frame->global_order_idx);
  }
  if (ref_update_type == RefUpdateType::kNone) {
    gop_frame->update_ref_idx = -1;
  } else {
    const int ref_idx = AllocateRefIdx();
    gop_frame->update_ref_idx = ref_idx;
    switch (ref_update_type) {
      case RefUpdateType::kForward: forward_stack_.push_back(ref_idx); break;
      case RefUpdateType::kBackward: backward_queue_.push_back(ref_idx); break;
      case RefUpdateType::kLast: last_queue_.push_back(ref_idx); break;
      case RefUpdateType::kNone: break;
    }
    ref_frame_table_[ref_idx] = *gop_frame;
  }
}

}  // namespace aom
