// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "uvkc/vulkan/command_buffer.h"

#include "absl/status/status.h"
#include "uvkc/vulkan/status_util.h"
#include "uvkc/vulkan/timestamp_query_pool.h"

namespace uvkc {
namespace vulkan {

CommandBuffer::CommandBuffer(VkDevice device, VkCommandBuffer command_buffer,
                             const DynamicSymbols &symbols)
    : command_buffer_(command_buffer), device_(device), symbols_(symbols) {}

CommandBuffer::~CommandBuffer() = default;

VkCommandBuffer CommandBuffer::command_buffer() const {
  return command_buffer_;
}

absl::Status CommandBuffer::Begin() {
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = nullptr;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = nullptr;
  return VkResultToStatus(
      symbols_.vkBeginCommandBuffer(command_buffer_, &begin_info));
}

absl::Status CommandBuffer::End() {
  return VkResultToStatus(symbols_.vkEndCommandBuffer(command_buffer_));
}

absl::Status CommandBuffer::Reset() {
  // We don't release the resources when resetting the command buffer. The
  // assumption behind this is that the command buffer will be used in some sort
  // of benchmarking loop so each iteration/recording requires the same
  // resource.
  return VkResultToStatus(
      symbols_.vkResetCommandBuffer(command_buffer_, /*flags=*/0));
}

void CommandBuffer::CopyBuffer(const Buffer &src_buffer, size_t src_offset,
                               const Buffer &dst_buffer, size_t dst_offset,
                               size_t length) {
  VkBufferCopy region = {};
  region.srcOffset = src_offset;
  region.dstOffset = dst_offset;
  region.size = length;
  symbols_.vkCmdCopyBuffer(command_buffer_, src_buffer.buffer(),
                           dst_buffer.buffer(),
                           /*regionCount=*/1, &region);
}

void CommandBuffer::BindPipelineAndDescriptorSets(
    const Pipeline &pipeline,
    absl::Span<const BoundDescriptorSet> bound_descriptor_sets) {
  symbols_.vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                             pipeline.pipeline());

  for (const auto &descriptor_set : bound_descriptor_sets) {
    symbols_.vkCmdBindDescriptorSets(
        command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline.pipeline_layout(), descriptor_set.index,
        /*descriptorSetCount=*/1,
        /*pDescriptorSets=*/&descriptor_set.set,
        /*dynamicOffsetCount=*/0,
        /*pDynamicOffsets=*/nullptr);
  }
}

void CommandBuffer::ResetQueryPool(const TimestampQueryPool &query_pool) {
  symbols_.vkCmdResetQueryPool(command_buffer_, query_pool.query_pool(),
                               /*firstQuery=*/0,
                               /*queryCount=*/query_pool.query_count());
}

void CommandBuffer::WriteTimestamp(const TimestampQueryPool &query_pool,
                                   VkPipelineStageFlagBits pipeline_stage,
                                   uint32_t query_index) {
  symbols_.vkCmdWriteTimestamp(command_buffer_, pipeline_stage,
                               query_pool.query_pool(), query_index);
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
  symbols_.vkCmdDispatch(command_buffer_, x, y, z);
}

}  // namespace vulkan
}  // namespace uvkc
