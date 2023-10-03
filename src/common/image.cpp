#include "svulkan2/common/image.h"
#include <ktx.h>
#include <ktxvulkan.h>
#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>
#pragma GCC diagnostic pop

namespace svulkan2 {
std::vector<uint8_t> loadImage(std::string const &filename, int &width, int &height, int &channels,
                               int desiredChannels = 0) {
  if (desiredChannels != 0 && desiredChannels != 1 && desiredChannels != 4) {
    throw std::runtime_error("image can only be loaded with 1 or 4 channels");
  }

  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!data) {
    throw std::runtime_error("failed to load image: " + filename);
  }

  std::vector<uint8_t> dataVector;

  if ((channels == 1 && desiredChannels != 4) || desiredChannels == 1) {
    dataVector.reserve(width * height);
    for (uint32_t i = 0; i < width * height; ++i) {
      dataVector.push_back(data[4 * i]);
    }
  } else {
    channels = 4;
    dataVector = std::vector<uint8_t>(data, data + width * height * channels);
  }

  stbi_image_free(data);
  return dataVector;
}

std::vector<uint8_t> loadImageFromMemory(unsigned char *buffer, int len, int &width, int &height,
                                         int &channels) {
  unsigned char *data =
      stbi_load_from_memory(buffer, len, &width, &height, &channels, STBI_rgb_alpha);
  if (!data) {
    throw std::runtime_error("failed to load image from memory");
  }

  std::vector<uint8_t> dataVector;
  if (channels == 1) {
    dataVector.reserve(width * height);
    for (uint32_t i = 0; i < width * height; ++i) {
      dataVector.push_back(data[4 * i]);
    }
  } else {
    channels = 4;
    dataVector = std::vector<uint8_t>(data, data + width * height * channels);
  }

  stbi_image_free(data);
  return dataVector;
}

std::vector<uint8_t> loadKTXImage(std::string const &filename, int &width, int &height,
                                  int &levels, int &faces, int &layers, vk::Format &format) {
  std::vector<uint8_t> data;

  ktxTexture *texture;
  KTX_error_code result;
  ktx_size_t offset;
  ktx_uint8_t *image;

  result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                          &texture);
  if (result != KTX_SUCCESS) {
    throw std::runtime_error("failed to load ktx texture " + filename);
  }

  width = texture->baseWidth;
  height = texture->baseHeight;
  if (texture->numDimensions != 2) {
    throw std::runtime_error("failed to load ktx texture " + filename +
                             ": only 2D textures are supported.");
  }

  format = vk::Format(ktxTexture_GetVkFormat(texture));

  levels = texture->numLevels;
  layers = texture->numLayers;
  faces = texture->numFaces;

  for (uint32_t layer = 0; layer < texture->numLayers; ++layer) {
    for (uint32_t face = 0; face < texture->numFaces; ++face) {
      for (uint32_t level = 0; level < texture->numLevels; ++level) {
        ktxTexture_GetImageOffset(texture, level, layer, face, &offset);
        size_t size = ktxTexture_GetImageSize(texture, level);
        image = ktxTexture_GetData(texture) + offset;
        std::copy(image, image + size, std::back_inserter(data));
      }
    }
  }

  ktxTexture_Destroy(texture);
  return data;
}

uint32_t computeMipLevelSize(vk::Extent3D extent, uint32_t level) {
  extent = computeMipLevelExtent(extent, level);
  return extent.width * extent.height * extent.depth;
}

vk::Extent3D computeMipLevelExtent(vk::Extent3D extent, uint32_t level) {
  uint32_t width = extent.width;
  uint32_t height = extent.height;
  uint32_t depth = extent.depth;
  for (uint32_t i = 0; i < level; ++i) {
    width = std::max(width / 2, 1u);
    height = std::max(height / 2, 1u);
    depth = std::max(depth / 2, 1u);
  }
  return {width, height, depth};
}

} // namespace svulkan2
