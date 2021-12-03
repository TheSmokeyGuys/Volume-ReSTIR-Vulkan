#ifndef __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__
#define __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__

#include <array>
#include <bitset>

namespace volume_restir {

enum QueueFlags {
  COMPUTE,
  GRAPHICS,
  TRANSFER,
  PRESENT,
};

namespace vkq_index {
const uint32_t kComputeIdx  = 1 << 0;
const uint32_t kGraphicsIdx = 1 << 1;
const uint32_t kTransferIdx = 1 << 2;
const uint32_t kPresentIdx  = 1 << 3;
}  // namespace vkq_index

using QueueFlagBits      = std::bitset<sizeof(QueueFlags)>;
using QueueFamilyIndices = std::array<int, sizeof(QueueFlags)>;

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_UTILS_VKQUEUE_UTILS_HPP__ */
