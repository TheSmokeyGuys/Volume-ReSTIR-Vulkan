#ifndef __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__
#define __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__

#include "VkBootstrap.h"

namespace volume_restir {
namespace vkq_index {
const uint32_t kComputeIdx  = static_cast<uint32_t>(vkb::QueueType::compute);
const uint32_t kGraphicsIdx = static_cast<uint32_t>(vkb::QueueType::graphics);
const uint32_t kTransferIdx = static_cast<uint32_t>(vkb::QueueType::transfer);
const uint32_t kPresentIdx  = static_cast<uint32_t>(vkb::QueueType::present);
}  // namespace vkq_index
}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__ */
