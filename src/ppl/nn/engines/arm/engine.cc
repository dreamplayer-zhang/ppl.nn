// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
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

#include "ppl/nn/engines/arm/engine.h"
#include <stdarg.h>
#include "ppl/nn/engines/arm/engine_context.h"
#include "ppl/nn/engines/arm/optimizer/opt_kernel_creator_manager.h"
#include "ppl/nn/engines/arm/optimizer/opt_graph.h"
#include "ppl/nn/common/logger.h"
#include "ppl/nn/engines/utils.h"

#if defined(__linux__) && defined(PPLNN_USE_NUMA)
#include <numa.h>
#endif

using namespace std;
using namespace ppl::common;

namespace ppl { namespace nn { namespace arm {

RetCode ArmEngine::Init(const ArmEngineOptions& options) {
    options_ = options;

#ifndef PPLNN_USE_ARMV8_2_FP16
    if (options_.forward_precision == ppl::common::DATATYPE_FLOAT16) {
        LOG(ERROR) << "current build not support FP16.";
        return RC_UNSUPPORTED;
    }
#endif

    if (options_.forward_precision != ppl::common::DATATYPE_FLOAT32 &&
        options_.forward_precision != ppl::common::DATATYPE_FLOAT16) {
        LOG(ERROR) << "arm engine only support fp16 & fp32 forward precision.";
        return RC_INVALID_VALUE;
    }

    BindNumaNode(options_.numa_node_id); // TODO: move to runtime init
    return RC_SUCCESS;
}

RetCode ArmEngine::BindNumaNode(int32_t numa_node_id) const {
    if (numa_node_id < 0) {
        return RC_SUCCESS; // not bind numa node
    }
#if defined(__linux__) && defined(PPLNN_USE_NUMA)
    if (numa_available() < 0) {
        LOG(WARNING) << "NUMA API check failed. current system not support NUMA API. engine will not bind numa.";
        return RC_UNSUPPORTED;
    }
    const int32_t max_numa_node_id = numa_max_node();
    if (numa_node_id > max_numa_node_id) {
        return RC_SUCCESS; // invalid numa_node_id, will not bind numa node
    }
    if (0 != numa_run_on_node(numa_node_id)) { // bind cpu task
        LOG(WARNING) << "numa bind failed.";
        return RC_UNSUPPORTED;
    }
    numa_set_preferred(numa_node_id); // bind memory alloc
    LOG(INFO) << "successfully bind engine to numa node " << numa_node_id << ".";
    return RC_SUCCESS;
#else
    LOG(WARNING) << "current build does not support NUMA. will not bind to numa node.";
    return RC_UNSUPPORTED;
#endif
}

EngineContext* ArmEngine::CreateEngineContext() {
    return new ArmEngineContext(device_.GetISA(), options_.mm_policy);
}

bool ArmEngine::Supports(const ir::Node* node) const {
    auto& type = node->GetType();
    return (OptKernelCreatorManager::Instance()->Find(type.domain, type.name, type.version) != nullptr);
}

RetCode ArmEngine::DoOptimize(ir::Graph* graph, utils::SharedResource* resource, RuntimePartitionInfo* info) {
    OptGraph opt_graph;
    auto status = opt_graph.Init(graph, resource, info, &options_);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "init OptGraph failed: " << GetRetCodeStr(status);
        return status;
    }

    status = opt_graph.DoOptimize(&device_);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "OptGraph DoOptimize failed: " << GetRetCodeStr(status);
        return status;
    }
    return RC_SUCCESS;
}

RetCode ArmEngine::ProcessGraph(utils::SharedResource* resource, ir::Graph* graph, RuntimePartitionInfo* info) {
    auto status = DoOptimize(graph, resource, info);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "DoOptimize failed: " << GetRetCodeStr(status);
        return status;
    }

    status = utils::LoadConstants(*graph, &device_, &info->constants);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "FillConstants failed: " << GetRetCodeStr(status);
        return status;
    }

    return RC_SUCCESS;
}

#ifdef PPLNN_ENABLE_PMX_MODEL
RetCode ArmEngine::LoadConstant(edgeid_t eid, const void* data, uint64_t size, const TensorShape& shape,
                                RuntimeConstantInfo* info) {
    return utils::GenericLoadConstant(eid, data, size, shape, &device_, info, (size == 0));
}

OptKernel* ArmEngine::CreateOptKernel(const ir::Node* node) const {
    auto& type = node->GetType();
    auto creator = OptKernelCreatorManager::Instance()->Find(type.domain, type.name, type.version);
    if (!creator) {
        LOG(ERROR) << "cannot find creator for node[" << node->GetName() << "] of type[" << type.domain << ":"
                   << type.name << ":" << type.version << "]";
        return nullptr;
    }

    auto opt_kernel = creator(node);
    if (!opt_kernel) {
        LOG(ERROR) << "create kernel[" << node->GetName() << "] failed: oom.";
        return nullptr;
    }

    return opt_kernel;
}
#endif

ArmEngine::ConfHandlerFunc ArmEngine::conf_handlers_[] = {};

RetCode ArmEngine::Configure(uint32_t option, ...) {
    if (option >= ARM_CONF_MAX) {
        LOG(ERROR) << "invalid option[" << option << "] >= [" << ARM_CONF_MAX << "]";
        return RC_INVALID_VALUE;
    }

    va_list args;
    va_start(args, option);
    auto status = conf_handlers_[option](this, args);
    va_end(args);

    return status;
}
}}} // namespace ppl::nn::arm
