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

#ifndef AOM_AV1_ENCODER_ML_H_
#define AOM_AV1_ENCODER_ML_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config/av1_rtcd.h"

#define NN_MAX_HIDDEN_LAYERS 10
#define NN_MAX_NODES_PER_LAYER 128

struct NN_CONFIG {
  int num_inputs;         // Number of input nodes, i.e. features.
  int num_outputs;        // Number of output nodes.
  int num_hidden_layers;  // Number of hidden layers, maximum 10.
  // Number of nodes for each hidden layer.
  int num_hidden_nodes[NN_MAX_HIDDEN_LAYERS];
  // Weight parameters, indexed by layer.
  const float *weights[NN_MAX_HIDDEN_LAYERS + 1];
  // Bias parameters, indexed by layer.
  const float *bias[NN_MAX_HIDDEN_LAYERS + 1];
};
// Typedef from struct NN_CONFIG to NN_CONFIG is in rtcd_defs

#if CONFIG_NN_V2
// Fully-connectedly layer configuration
struct FC_LAYER {
  int num_inputs;   // Number of input nodes, i.e. features.
  int num_outputs;  // Number of output nodes.

  float *weights;         // Weight parameters.
  float *bias;            // Bias parameters.
  ACTIVATION activation;  // Activation function.

  float *output;  // The output array.
  float *dY;      // Gradient of outputs
  float *dW;      // Gradient of weights.
  float *db;      // Gradient of bias
};

// NN configure structure V2
struct NN_CONFIG_V2 {
  int counter;            // Counter for the input in one batch
  int num_hidden_layers;  // Number of hidden layers, max = 10.
  float *feature;         // Input feature
  FC_LAYER layer[NN_MAX_HIDDEN_LAYERS + 1];  // The layer array
  int num_logits;                            // Number of output nodes.
  float *logits;  // Raw prediction (same as output of final layer)
  LOSS loss;      // Loss function
};

// Calculate prediction based on the given input features and neural net config.
// Assume there are no more than NN_MAX_NODES_PER_LAYER nodes in each hidden
// layer.
void av1_nn_predict_v2(const float *features, NN_CONFIG_V2 *nn_config,
                       int reduce_prec, float *output);

// Back propagation on the given NN model.
void av1_nn_backprop(NN_CONFIG_V2 *nn_config, const int label);

// Back propagation on the given two NN models (!!Only for transform type).
void av1_nn_outer_product_backprop(NN_CONFIG_V2 *nn_config_hor,
                                   NN_CONFIG_V2 *nn_config_ver,
                                   const int label);

// Update the weights via gradient descent.
// mu: learning rate, usually chosen from 0.01~0.001.
void av1_nn_update(NN_CONFIG_V2 *nn_config, float mu);
#endif  // CONFIG_NN_V2

// Applies the softmax normalization function to the input
// to get a valid probability distribution in the output:
// output[i] = exp(input[i]) / sum_{k \in [0,n)}(exp(input[k]))
void av1_nn_softmax(const float *input, float *output, int n);

// Applies a precision reduction to output of av1_nn_predict to prevent
// mismatches between C and SIMD implementations.
void av1_nn_output_prec_reduce(float *const output, int num_output);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_ML_H_
