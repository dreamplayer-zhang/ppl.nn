// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for arithmeticitional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef __ST_PPL_KERNEL_ARM_SERVER_CHANNEL_SHUFFLE_NEON_CHANNEL_SHUFFLE_H_
#define __ST_PPL_KERNEL_ARM_SERVER_CHANNEL_SHUFFLE_NEON_CHANNEL_SHUFFLE_H_

#include "ppl/kernel/arm_server/common/general_include.h"

namespace ppl { namespace kernel { namespace arm_server { namespace neon {

ppl::common::RetCode channel_shuffle_concat_split(
    const ppl::nn::TensorShape *src0_shape,
    const ppl::nn::TensorShape *src1_shape,
    const ppl::nn::TensorShape *dst0_shape,
    const ppl::nn::TensorShape *dst1_shape,
    const void *src0,
    const void *src1,
    const int32_t group,
    void *dst0,
    void *dst1);

ppl::common::RetCode channel_shuffle_concat(
    const ppl::nn::TensorShape *src0_shape,
    const ppl::nn::TensorShape *src1_shape,
    const ppl::nn::TensorShape *dst_shape,
    const void *src0,
    const void *src1,
    const int32_t group,
    void *dst);

}}}}; // namespace ppl::kernel::arm_server::neon

#endif // __ST_PPL_KERNEL_ARM_SERVER_CHANNEL_SHUFFLE_NEON_CHANNEL_SHUFFLE_H_
