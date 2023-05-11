#include "svulkan2/shader/primitive_shadow.h"
#include "reflect.h"

namespace svulkan2 {
namespace shader {

void PrimitiveShadowPassParser::reflectSPV() {
  spirv_cross::Compiler vertComp(mVertSPVCode);
  std::vector<DescriptorSetDescription> vertDesc;
  try {
    mVertexInputLayout = parseVertexInput(vertComp);
    for (uint32_t i = 0; i < 4; ++i) {
      vertDesc.push_back(getDescriptorSetDescription(vertComp, i));
    }
  } catch (std::runtime_error const &err) {
    throw std::runtime_error("[vert]" + std::string(err.what()));
  }

  for (uint32_t i = 0, stop = false; i < 4; ++i) {
    if (vertDesc[i].type == UniformBindingType::eNone) {
      stop = true;
      continue;
    }
    if (stop == true) {
      throw std::runtime_error("primitive shadow: descriptor sets should use "
                               "consecutive integers starting from 0");
    }
    mDescriptorSetDescriptions.push_back(vertDesc[i]);
  }

  validate();
}

void PrimitiveShadowPassParser::validate() const {
  for (auto &desc : mDescriptorSetDescriptions) {
    if (desc.type != UniformBindingType::eObject &&
        desc.type != UniformBindingType::eLight) {
      throw std::runtime_error("primitive shadow: only object and light "
                               "buffers are allowed in a shadow pass");
    }
  }
}

std::optional<std::string>
PrimitiveShadowPassParser::getDepthRenderTargetName() const {
  return "ShadowDepth";
}

vk::UniqueRenderPass PrimitiveShadowPassParser::createRenderPass(
    vk::Device device, std::vector<vk::Format> const &colorFormats,
    vk::Format depthFormat,
    std::vector<std::pair<vk::ImageLayout, vk::ImageLayout>> const
        &colorTargetLayouts,
    std::pair<vk::ImageLayout, vk::ImageLayout> const &depthLayout,
    vk::SampleCountFlagBits sampleCount) const {
  std::vector<vk::AttachmentDescription> attachmentDescriptions;

  // Only Depth Attachment needed for Shadow Mapping
  attachmentDescriptions.push_back(vk::AttachmentDescription(
      vk::AttachmentDescriptionFlags(), depthFormat, sampleCount,
      depthLayout.first == vk::ImageLayout::eUndefined
          ? vk::AttachmentLoadOp::eClear
          : vk::AttachmentLoadOp::eLoad,
      vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      depthLayout.first,    // Initial Layout
      depthLayout.second)); // Final Layout

  vk::AttachmentReference depthAttachment(
      0, vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::SubpassDescription subpassDescription(
      {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 0, nullptr, nullptr,
      &depthAttachment);

  // TODO: compute a better dependency
  std::array<vk::SubpassDependency, 2> deps{
      vk::SubpassDependency(
          VK_SUBPASS_EXTERNAL, 0,
          vk::PipelineStageFlagBits::eColorAttachmentOutput |
              vk::PipelineStageFlagBits::eEarlyFragmentTests |
              vk::PipelineStageFlagBits::eLateFragmentTests,
          vk::PipelineStageFlagBits::eFragmentShader |
              vk::PipelineStageFlagBits::eColorAttachmentOutput,
          vk::AccessFlagBits::eColorAttachmentWrite |
              vk::AccessFlagBits::eDepthStencilAttachmentWrite,
          vk::AccessFlagBits::eShaderRead |
              vk::AccessFlagBits::eColorAttachmentWrite),
      vk::SubpassDependency(
          0, VK_SUBPASS_EXTERNAL,
          vk::PipelineStageFlagBits::eColorAttachmentOutput |
              vk::PipelineStageFlagBits::eEarlyFragmentTests |
              vk::PipelineStageFlagBits::eLateFragmentTests,
          vk::PipelineStageFlagBits::eFragmentShader |
              vk::PipelineStageFlagBits::eColorAttachmentOutput,
          vk::AccessFlagBits::eColorAttachmentWrite |
              vk::AccessFlagBits::eDepthStencilAttachmentWrite,
          vk::AccessFlagBits::eShaderRead |
              vk::AccessFlagBits::eColorAttachmentWrite),
  };

  return device.createRenderPassUnique(vk::RenderPassCreateInfo(
      {}, attachmentDescriptions.size(), attachmentDescriptions.data(), 1,
      &subpassDescription, 2, deps.data()));
}

vk::UniquePipeline PrimitiveShadowPassParser::createPipelineHelper(
    vk::Device device, vk::PipelineLayout layout, vk::RenderPass renderPass,
    vk::CullModeFlags cullMode, vk::FrontFace frontFace, bool alphaBlend,
    vk::SampleCountFlagBits sampleCount,
    std::map<std::string, SpecializationConstantValue> const
        &specializationConstantInfo,
    int primitiveType, float primitiveSize) const {

  // shaders
  vk::UniquePipelineCache pipelineCache =
      device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
  auto vsm = device.createShaderModuleUnique(
      {{}, mVertSPVCode.size() * sizeof(uint32_t), mVertSPVCode.data()});
  auto fsm = device.createShaderModuleUnique(
      {{}, mFragSPVCode.size() * sizeof(uint32_t), mFragSPVCode.data()});
  vk::UniqueShaderModule gsm;
  if (mGeomSPVCode.size()) {
    gsm = device.createShaderModuleUnique(
        {{}, mGeomSPVCode.size() * sizeof(uint32_t), mGeomSPVCode.data()});
  }

  std::vector<vk::PipelineShaderStageCreateInfo>
      pipelineShaderStageCreateInfos = {
          vk::PipelineShaderStageCreateInfo(
              vk::PipelineShaderStageCreateFlags(),
              vk::ShaderStageFlagBits::eVertex, vsm.get(), "main", nullptr),
          vk::PipelineShaderStageCreateInfo(
              vk::PipelineShaderStageCreateFlags(),
              vk::ShaderStageFlagBits::eFragment, fsm.get(), "main", nullptr)};
  if (gsm) {
    pipelineShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eGeometry, gsm.get(), "main", nullptr));
  }

  // vertex input
  auto vertexInputBindingDescriptions =
      mVertexInputLayout->computeVertexInputBindingDescriptions();
  auto vertexInputAttributeDescriptions =
      mVertexInputLayout->computeVertexInputAttributesDescriptions();
  vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount =
      vertexInputBindingDescriptions.size();
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions =
      vertexInputBindingDescriptions.data();
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
      vertexInputAttributeDescriptions.size();
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexInputAttributeDescriptions.data();

  // input assembly
  vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
      vk::PipelineInputAssemblyStateCreateFlags(),
      primitiveType == 0 ? vk::PrimitiveTopology::ePointList
                         : vk::PrimitiveTopology::eLineList);

  // viewport
  vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
      vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

  // rasterization
  vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
      vk::PipelineRasterizationStateCreateFlags(), false, false,
      vk::PolygonMode::eFill, cullMode, frontFace, false, 0.0f, 0.0f, 0.f,
      primitiveSize);

  // multisample
  vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
      {}, sampleCount};

  // stencil
  vk::StencilOpState stencilOpState{};
  vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
      vk::PipelineDepthStencilStateCreateFlags(), true, true,
      vk::CompareOp::eLessOrEqual, false, false, stencilOpState,
      stencilOpState);

  // dynamic
  vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport,
                                       vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
      vk::PipelineDynamicStateCreateFlags(), 2, dynamicStates);

  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      vk::PipelineCreateFlags(), pipelineShaderStageCreateInfos.size(),
      pipelineShaderStageCreateInfos.data(),
      &pipelineVertexInputStateCreateInfo,
      &pipelineInputAssemblyStateCreateInfo, nullptr,
      &pipelineViewportStateCreateInfo, &pipelineRasterizationStateCreateInfo,
      &pipelineMultisampleStateCreateInfo, &pipelineDepthStencilStateCreateInfo,
      nullptr, &pipelineDynamicStateCreateInfo, layout, renderPass);
  return device
      .createGraphicsPipelineUnique(pipelineCache.get(),
                                    graphicsPipelineCreateInfo)
      .value;
}

std::vector<UniformBindingType>
PrimitiveShadowPassParser::getUniformBindingTypes() const {
  std::vector<UniformBindingType> result;
  for (auto &desc : mDescriptorSetDescriptions) {
    result.push_back(desc.type);
  }
  return result;
}

vk::UniquePipeline PointShadowParser::createPipeline(
    vk::Device device, vk::PipelineLayout layout, vk::RenderPass renderPass,
    vk::CullModeFlags cullMode, vk::FrontFace frontFace, bool alphaBlend,
    vk::SampleCountFlagBits sampleCount,
    std::map<std::string, SpecializationConstantValue> const
        &specializationConstantInfo) const {
  return createPipelineHelper(device, layout, renderPass, cullMode, frontFace,
                              alphaBlend, sampleCount,
                              specializationConstantInfo, 0, 0.f);
}

} // namespace shader
} // namespace svulkan2
