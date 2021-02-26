/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "av1/encoder/context_tree.h"
#include "av1/encoder/encoder.h"

static const BLOCK_SIZE square[MAX_SB_SIZE_LOG2 - 1] = {
  BLOCK_4X4, BLOCK_8X8, BLOCK_16X16, BLOCK_32X32, BLOCK_64X64, BLOCK_128X128,
};

void av1_copy_tree_context(PICK_MODE_CONTEXT *dst_ctx,
                           PICK_MODE_CONTEXT *src_ctx) {
  dst_ctx->mic = src_ctx->mic;
  dst_ctx->mbmi_ext_best = src_ctx->mbmi_ext_best;

  dst_ctx->num_4x4_blk = src_ctx->num_4x4_blk;
  dst_ctx->skippable = src_ctx->skippable;
#if CONFIG_INTERNAL_STATS && !CONFIG_NEW_REF_SIGNALING
  dst_ctx->best_mode_index = src_ctx->best_mode_index;
#endif  // CONFIG_INTERNAL_STATS && !CONFIG_NEW_REF_SIGNALING

  memcpy(dst_ctx->blk_skip, src_ctx->blk_skip,
         sizeof(uint8_t) * src_ctx->num_4x4_blk);
  av1_copy_array(dst_ctx->tx_type_map, src_ctx->tx_type_map,
                 src_ctx->num_4x4_blk);

  dst_ctx->hybrid_pred_diff = src_ctx->hybrid_pred_diff;
  dst_ctx->comp_pred_diff = src_ctx->comp_pred_diff;
  dst_ctx->single_pred_diff = src_ctx->single_pred_diff;

  dst_ctx->rd_stats = src_ctx->rd_stats;
  dst_ctx->rd_mode_is_ready = src_ctx->rd_mode_is_ready;
#if CONFIG_EXT_RECUR_PARTITIONS
  for (int i = 0; i < 2; ++i) {
    memcpy(dst_ctx->color_index_map[i], src_ctx->color_index_map[i],
           sizeof(src_ctx->color_index_map[i][0]) * src_ctx->num_4x4_blk * 16);
  }
#endif  // CONFIG_EXT_RECUR_PARTITIONS
}

void av1_setup_shared_coeff_buffer(AV1_COMMON *cm,
                                   PC_TREE_SHARED_BUFFERS *shared_bufs) {
  for (int i = 0; i < 3; i++) {
    const int max_num_pix = MAX_SB_SIZE * MAX_SB_SIZE;
    CHECK_MEM_ERROR(cm, shared_bufs->coeff_buf[i],
                    aom_memalign(32, max_num_pix * sizeof(tran_low_t)));
    CHECK_MEM_ERROR(cm, shared_bufs->qcoeff_buf[i],
                    aom_memalign(32, max_num_pix * sizeof(tran_low_t)));
    CHECK_MEM_ERROR(cm, shared_bufs->dqcoeff_buf[i],
                    aom_memalign(32, max_num_pix * sizeof(tran_low_t)));
  }
}

void av1_free_shared_coeff_buffer(PC_TREE_SHARED_BUFFERS *shared_bufs) {
  for (int i = 0; i < 3; i++) {
    aom_free(shared_bufs->coeff_buf[i]);
    aom_free(shared_bufs->qcoeff_buf[i]);
    aom_free(shared_bufs->dqcoeff_buf[i]);
    shared_bufs->coeff_buf[i] = NULL;
    shared_bufs->qcoeff_buf[i] = NULL;
    shared_bufs->dqcoeff_buf[i] = NULL;
  }
}

PICK_MODE_CONTEXT *av1_alloc_pmc(const AV1_COMMON *cm, int mi_row, int mi_col,
                                 BLOCK_SIZE bsize, PC_TREE *parent,
                                 PARTITION_TYPE parent_partition, int index,
                                 int subsampling_x, int subsampling_y,
                                 PC_TREE_SHARED_BUFFERS *shared_bufs) {
  PICK_MODE_CONTEXT *ctx = NULL;
  struct aom_internal_error_info error;

  AOM_CHECK_MEM_ERROR(&error, ctx, aom_calloc(1, sizeof(*ctx)));
  ctx->rd_mode_is_ready = 0;
  ctx->parent = parent;
  ctx->index = index;
  set_chroma_ref_info(mi_row, mi_col, index, bsize, &ctx->chroma_ref_info,
                      parent ? &parent->chroma_ref_info : NULL,
                      parent ? parent->block_size : BLOCK_INVALID,
                      parent_partition, subsampling_x, subsampling_y);
  ctx->mic.chroma_ref_info = ctx->chroma_ref_info;

  const int num_planes = av1_num_planes(cm);
  const int num_pix = block_size_wide[bsize] * block_size_high[bsize];
  const int num_blk = num_pix / 16;

  AOM_CHECK_MEM_ERROR(&error, ctx->blk_skip,
                      aom_calloc(num_blk, sizeof(*ctx->blk_skip)));
  AOM_CHECK_MEM_ERROR(&error, ctx->tx_type_map,
                      aom_calloc(num_blk, sizeof(*ctx->tx_type_map)));
  ctx->num_4x4_blk = num_blk;

  for (int i = 0; i < num_planes; ++i) {
    ctx->coeff[i] = shared_bufs->coeff_buf[i];
    ctx->qcoeff[i] = shared_bufs->qcoeff_buf[i];
    ctx->dqcoeff[i] = shared_bufs->dqcoeff_buf[i];
    AOM_CHECK_MEM_ERROR(&error, ctx->eobs[i],
                        aom_memalign(32, num_blk * sizeof(*ctx->eobs[i])));
    AOM_CHECK_MEM_ERROR(
        &error, ctx->txb_entropy_ctx[i],
        aom_memalign(32, num_blk * sizeof(*ctx->txb_entropy_ctx[i])));
  }

  if (num_pix <= MAX_PALETTE_SQUARE) {
    for (int i = 0; i < 2; ++i) {
      AOM_CHECK_MEM_ERROR(
          &error, ctx->color_index_map[i],
          aom_memalign(32, num_pix * sizeof(*ctx->color_index_map[i])));
    }
  }

  return ctx;
}

void av1_free_pmc(PICK_MODE_CONTEXT *ctx, int num_planes) {
  if (ctx == NULL) return;

  aom_free(ctx->blk_skip);
  ctx->blk_skip = NULL;
  aom_free(ctx->tx_type_map);
  for (int i = 0; i < num_planes; ++i) {
    ctx->coeff[i] = NULL;
    ctx->qcoeff[i] = NULL;
    ctx->dqcoeff[i] = NULL;
    aom_free(ctx->eobs[i]);
    ctx->eobs[i] = NULL;
    aom_free(ctx->txb_entropy_ctx[i]);
    ctx->txb_entropy_ctx[i] = NULL;
  }

  for (int i = 0; i < 2; ++i) {
    aom_free(ctx->color_index_map[i]);
    ctx->color_index_map[i] = NULL;
  }

  aom_free(ctx);
}

PC_TREE *av1_alloc_pc_tree_node(int mi_row, int mi_col, BLOCK_SIZE bsize,
                                PC_TREE *parent,
                                PARTITION_TYPE parent_partition, int index,
                                int is_last, int subsampling_x,
                                int subsampling_y) {
  PC_TREE *pc_tree = NULL;
  struct aom_internal_error_info error;

  AOM_CHECK_MEM_ERROR(&error, pc_tree, aom_calloc(1, sizeof(*pc_tree)));

  pc_tree->mi_row = mi_row;
  pc_tree->mi_col = mi_col;
  pc_tree->parent = parent;
  pc_tree->index = index;
  pc_tree->partitioning = PARTITION_NONE;
  pc_tree->block_size = bsize;
  pc_tree->index = 0;
  pc_tree->is_last_subblock = is_last;
  set_chroma_ref_info(mi_row, mi_col, index, bsize, &pc_tree->chroma_ref_info,
                      parent ? &parent->chroma_ref_info : NULL,
                      parent ? parent->block_size : BLOCK_INVALID,
                      parent_partition, subsampling_x, subsampling_y);

  pc_tree->none = NULL;
  for (int i = 0; i < 2; ++i) {
    pc_tree->horizontal[i] = NULL;
    pc_tree->vertical[i] = NULL;
  }
#if CONFIG_EXT_RECUR_PARTITIONS
  for (int i = 0; i < 3; ++i) {
    pc_tree->horizontal3[i] = NULL;
    pc_tree->vertical3[i] = NULL;
  }
#else
  for (int i = 0; i < 3; ++i) {
    pc_tree->horizontala[i] = NULL;
    pc_tree->horizontalb[i] = NULL;
    pc_tree->verticala[i] = NULL;
    pc_tree->verticalb[i] = NULL;
  }
#endif  // CONFIG_EXT_RECUR_PARTITIONS
  for (int i = 0; i < 4; ++i) {
#if !CONFIG_EXT_RECUR_PARTITIONS
    pc_tree->horizontal4[i] = NULL;
    pc_tree->vertical4[i] = NULL;
#endif  // !CONFIG_EXT_RECUR_PARTITIONS
    pc_tree->split[i] = NULL;
  }

  return pc_tree;
}

#define FREE_PMC_NODE(CTX)         \
  do {                             \
    av1_free_pmc(CTX, num_planes); \
    CTX = NULL;                    \
  } while (0)

void av1_free_pc_tree_recursive(PC_TREE *pc_tree, int num_planes, int keep_best,
                                int keep_none) {
  if (pc_tree == NULL) return;

  const PARTITION_TYPE partition = pc_tree->partitioning;

  if (!keep_none && (!keep_best || (partition != PARTITION_NONE)))
    FREE_PMC_NODE(pc_tree->none);

  for (int i = 0; i < 2; ++i) {
#if CONFIG_EXT_RECUR_PARTITIONS
    if ((!keep_best || (partition != PARTITION_HORZ)) &&
        pc_tree->horizontal[i] != NULL) {
      av1_free_pc_tree_recursive(pc_tree->horizontal[i], num_planes, 0, 0);
      pc_tree->horizontal[i] = NULL;
    }
    if ((!keep_best || (partition != PARTITION_VERT)) &&
        pc_tree->vertical[i] != NULL) {
      av1_free_pc_tree_recursive(pc_tree->vertical[i], num_planes, 0, 0);
      pc_tree->vertical[i] = NULL;
    }
#else
    if (!keep_best || (partition != PARTITION_HORZ))
      FREE_PMC_NODE(pc_tree->horizontal[i]);
    if (!keep_best || (partition != PARTITION_VERT))
      FREE_PMC_NODE(pc_tree->vertical[i]);
#endif  // CONFIG_EXT_RECUR_PARTITIONS
  }
#if CONFIG_EXT_RECUR_PARTITIONS
  for (int i = 0; i < 3; ++i) {
    if ((!keep_best || (partition != PARTITION_HORZ_3)) &&
        pc_tree->horizontal3[i] != NULL) {
      av1_free_pc_tree_recursive(pc_tree->horizontal3[i], num_planes, 0, 0);
      pc_tree->horizontal3[i] = NULL;
    }
    if ((!keep_best || (partition != PARTITION_VERT_3)) &&
        pc_tree->vertical3[i] != NULL) {
      av1_free_pc_tree_recursive(pc_tree->vertical3[i], num_planes, 0, 0);
      pc_tree->vertical3[i] = NULL;
    }
  }
#else
  for (int i = 0; i < 3; ++i) {
    if (!keep_best || (partition != PARTITION_HORZ_A))
      FREE_PMC_NODE(pc_tree->horizontala[i]);
    if (!keep_best || (partition != PARTITION_HORZ_B))
      FREE_PMC_NODE(pc_tree->horizontalb[i]);
    if (!keep_best || (partition != PARTITION_VERT_A))
      FREE_PMC_NODE(pc_tree->verticala[i]);
    if (!keep_best || (partition != PARTITION_VERT_B))
      FREE_PMC_NODE(pc_tree->verticalb[i]);
  }
  for (int i = 0; i < 4; ++i) {
    if (!keep_best || (partition != PARTITION_HORZ_4))
      FREE_PMC_NODE(pc_tree->horizontal4[i]);
    if (!keep_best || (partition != PARTITION_VERT_4))
      FREE_PMC_NODE(pc_tree->vertical4[i]);
  }
#endif  // CONFIG_EXT_RECUR_PARTITIONS

  if (!keep_best || (partition != PARTITION_SPLIT)) {
    for (int i = 0; i < 4; ++i) {
      if (pc_tree->split[i] != NULL) {
        av1_free_pc_tree_recursive(pc_tree->split[i], num_planes, 0, 0);
        // if (keep_best) aom_free(pc_tree->split[i]);
        pc_tree->split[i] = NULL;
      }
    }
  }

  if (!keep_best && !keep_none) aom_free(pc_tree);
}

#if CONFIG_EXT_RECUR_PARTITIONS
void av1_copy_pc_tree_recursive(const AV1_COMMON *cm, PC_TREE *dst,
                                PC_TREE *src, int ss_x, int ss_y,
                                PC_TREE_SHARED_BUFFERS *shared_bufs,
                                int num_planes) {
  // Copy the best partition type. For basic information like bsize and index,
  // we assume they have been set properly when initializing the dst PC_TREE
  dst->partitioning = src->partitioning;
  dst->rd_cost = src->rd_cost;
  const BLOCK_SIZE bsize = dst->block_size;
  const BLOCK_SIZE subsize = get_partition_subsize(bsize, src->partitioning);
  const int mi_row = src->mi_row;
  const int mi_col = src->mi_col;

  switch (src->partitioning) {
    // PARTITION_NONE
    case PARTITION_NONE:
      if (dst->none) av1_free_pmc(dst->none, num_planes);
      dst->none = NULL;
      if (src->none) {
        dst->none = av1_alloc_pmc(cm, mi_row, mi_col, bsize, dst,
                                  PARTITION_NONE, 0, ss_x, ss_y, shared_bufs);
        av1_copy_tree_context(dst->none, src->none);
      }
      break;
    // PARTITION_SPLIT
    case PARTITION_SPLIT:
      if (is_partition_valid(bsize, PARTITION_SPLIT)) {
        for (int i = 0; i < 4; ++i) {
          if (dst->split[i]) {
            av1_free_pc_tree_recursive(dst->split[i], num_planes, 0, 0);
            dst->split[i] = NULL;
          }
          if (src->split[i]) {
            const int x_idx = (i & 1) * (mi_size_wide[bsize] >> 1);
            const int y_idx = (i >> 1) * (mi_size_high[bsize] >> 1);
            dst->split[i] = av1_alloc_pc_tree_node(
                mi_row + y_idx, mi_col + x_idx, subsize, dst, PARTITION_SPLIT,
                i, i == 3, ss_x, ss_y);
            av1_copy_pc_tree_recursive(cm, dst->split[i], src->split[i], ss_x,
                                       ss_y, shared_bufs, num_planes);
          }
        }
      }
      break;
    // PARTITION_HORZ
    case PARTITION_HORZ:
      if (is_partition_valid(bsize, PARTITION_HORZ)) {
        for (int i = 0; i < 2; ++i) {
          if (dst->horizontal[i]) {
            av1_free_pc_tree_recursive(dst->horizontal[i], num_planes, 0, 0);
            dst->horizontal[i] = NULL;
          }
          if (src->horizontal[i]) {
            const int this_mi_row = mi_row + i * (mi_size_high[bsize] >> 1);
            dst->horizontal[i] =
                av1_alloc_pc_tree_node(this_mi_row, mi_col, subsize, dst,
                                       PARTITION_HORZ, i, i == 1, ss_x, ss_y);
            av1_copy_pc_tree_recursive(cm, dst->horizontal[i],
                                       src->horizontal[i], ss_x, ss_y,
                                       shared_bufs, num_planes);
          }
        }
      }
      break;
    // PARTITION_VERT
    case PARTITION_VERT:
      if (is_partition_valid(bsize, PARTITION_VERT)) {
        for (int i = 0; i < 2; ++i) {
          if (dst->vertical[i]) {
            av1_free_pc_tree_recursive(dst->vertical[i], num_planes, 0, 0);
            dst->vertical[i] = NULL;
          }
          if (src->vertical[i]) {
            const int this_mi_col = mi_col + i * (mi_size_wide[bsize] >> 1);
            dst->vertical[i] =
                av1_alloc_pc_tree_node(mi_row, this_mi_col, subsize, dst,
                                       PARTITION_VERT, i, i == 1, ss_x, ss_y);
            av1_copy_pc_tree_recursive(cm, dst->vertical[i], src->vertical[i],
                                       ss_x, ss_y, shared_bufs, num_planes);
          }
        }
      }
      break;
    // PARTITION_HORZ_3
    case PARTITION_HORZ_3:
      if (is_partition_valid(bsize, PARTITION_HORZ_3)) {
        const int mi_rows[3] = { mi_row, mi_row + (mi_size_high[bsize] >> 2),
                                 mi_row + (mi_size_high[bsize] >> 2) * 3 };
        const BLOCK_SIZE subsizes[3] = {
          subsize, get_partition_subsize(bsize, PARTITION_HORZ), subsize
        };

        for (int i = 0; i < 3; ++i) {
          if (dst->horizontal3[i]) {
            av1_free_pc_tree_recursive(dst->horizontal3[i], num_planes, 0, 0);
            dst->horizontal3[i] = NULL;
          }
          if (src->horizontal3[i]) {
            dst->horizontal3[i] =
                av1_alloc_pc_tree_node(mi_rows[i], mi_col, subsizes[i], dst,
                                       PARTITION_HORZ_3, i, i == 2, ss_x, ss_y);
            av1_copy_pc_tree_recursive(cm, dst->horizontal3[i],
                                       src->horizontal3[i], ss_x, ss_y,
                                       shared_bufs, num_planes);
          }
        }
      }
      break;
    // PARTITION_VERT_3
    case PARTITION_VERT_3:
      if (is_partition_valid(bsize, PARTITION_VERT_3)) {
        const int mi_cols[3] = { mi_col, mi_col + (mi_size_wide[bsize] >> 2),
                                 mi_col + (mi_size_wide[bsize] >> 2) * 3 };
        const BLOCK_SIZE subsizes[3] = {
          subsize, get_partition_subsize(bsize, PARTITION_VERT), subsize
        };

        for (int i = 0; i < 3; ++i) {
          if (dst->vertical3[i]) {
            av1_free_pc_tree_recursive(dst->vertical3[i], num_planes, 0, 0);
            dst->vertical3[i] = NULL;
          }
          if (src->vertical3[i]) {
            dst->vertical3[i] =
                av1_alloc_pc_tree_node(mi_row, mi_cols[i], subsizes[i], dst,
                                       PARTITION_VERT_3, i, i == 2, ss_x, ss_y);
            av1_copy_pc_tree_recursive(cm, dst->vertical3[i], src->vertical3[i],
                                       ss_x, ss_y, shared_bufs, num_planes);
          }
        }
      }
      break;
    default: assert(0 && "Not a valid partition."); break;
  }
}
#endif  // CONFIG_EXT_RECUR_PARTITIONS

static AOM_INLINE int get_pc_tree_nodes(const int is_sb_size_128,
                                        int stat_generation_stage) {
  const int tree_nodes_inc = is_sb_size_128 ? 1024 : 0;
  const int tree_nodes =
      stat_generation_stage ? 1 : (tree_nodes_inc + 256 + 64 + 16 + 4 + 1);
  return tree_nodes;
}

void av1_setup_sms_tree(AV1_COMP *const cpi, ThreadData *td) {
  AV1_COMMON *const cm = &cpi->common;
  const int stat_generation_stage = is_stat_generation_stage(cpi);
  const int is_sb_size_128 = cm->seq_params.sb_size == BLOCK_128X128;
  const int tree_nodes =
      get_pc_tree_nodes(is_sb_size_128, stat_generation_stage);
  int sms_tree_index = 0;
  SIMPLE_MOTION_DATA_TREE *this_sms;
  int square_index = 1;
  int nodes;

  aom_free(td->sms_tree);
  CHECK_MEM_ERROR(cm, td->sms_tree,
                  aom_calloc(tree_nodes, sizeof(*td->sms_tree)));
  this_sms = &td->sms_tree[0];

  if (!stat_generation_stage) {
    const int leaf_factor = is_sb_size_128 ? 4 : 1;
    const int leaf_nodes = 256 * leaf_factor;

    // Sets up all the leaf nodes in the tree.
    for (sms_tree_index = 0; sms_tree_index < leaf_nodes; ++sms_tree_index) {
      SIMPLE_MOTION_DATA_TREE *const tree = &td->sms_tree[sms_tree_index];
      tree->block_size = square[0];
    }

    // Each node has 4 leaf nodes, fill each block_size level of the tree
    // from leafs to the root.
    for (nodes = leaf_nodes >> 2; nodes > 0; nodes >>= 2) {
      for (int i = 0; i < nodes; ++i) {
        SIMPLE_MOTION_DATA_TREE *const tree = &td->sms_tree[sms_tree_index];
        tree->block_size = square[square_index];
        for (int j = 0; j < 4; j++) tree->split[j] = this_sms++;
        ++sms_tree_index;
      }
      ++square_index;
    }
  } else {
    // Allocation for firstpass/LAP stage
    // TODO(Mufaddal): refactor square_index to use a common block_size macro
    // from firstpass.c
    SIMPLE_MOTION_DATA_TREE *const tree = &td->sms_tree[sms_tree_index];
    square_index = 2;
    tree->block_size = square[square_index];
  }

  // Set up the root node for the largest superblock size
  td->sms_root = &td->sms_tree[tree_nodes - 1];
}

void av1_free_sms_tree(ThreadData *td) {
  if (td->sms_tree != NULL) {
    aom_free(td->sms_tree);
    td->sms_tree = NULL;
  }
}
