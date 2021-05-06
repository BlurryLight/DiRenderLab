//
// Created by zhong on 2021/4/26.
//

#include "texture.hh"
#include "../third_party/stb_image.h"
#include "global.hh"
#include <memory>
#include <utility>
using namespace DRL;
using byte = uint8_t;
using img_array_t = std::variant<std::shared_ptr<float>, std::shared_ptr<byte>>;
struct LoadReturnType {
  img_array_t data_ptr{};
  bool gamma = false;
  int nchannels = -1;
  int height = -1;
  int width = -1;
};

std::tuple<GLenum, GLenum, GLenum>
get_internalf_format(const LoadReturnType &res) {
  // defualt value: OpenGL internal format:GL_RGB8
  // loaded image value: GL_RGB, data type:unsigned byte
  GLenum internal_format = GL_RGB8, img_format = GL_RGB,
         img_data_type = GL_UNSIGNED_BYTE;
  img_data_type = res.gamma ? GL_FLOAT : GL_UNSIGNED_BYTE;
  switch (res.nchannels) {
  case 1:
    internal_format = res.gamma ? GL_R16F : GL_R8;
    img_format = GL_RED;
    break;
  case 2:
    internal_format = res.gamma ? GL_RG16F : GL_RG8;
    img_format = GL_RG;
    break;
  case 3:
    internal_format = res.gamma ? GL_RGB16F : GL_RGB8;
    img_format = GL_RGB;
    break;
  case 4:
    internal_format = res.gamma ? GL_RGBA16F : GL_RGBA8;
    img_format = GL_RGBA;
    break;
  }
  return {internal_format, img_format, img_data_type};
}
static LoadReturnType TextureFromFile(const fs::path &path, bool gamma,
                                      bool flip) {
  // to make sure this renderer works correctly in SRGB space,
  // there will need much more efforts.
  if (flip) {
    stbi_set_flip_vertically_on_load(true);
  }
  int width, height, nrComponents;
  auto absolute_path = fs::absolute(path);
  //  auto data = std::shared_ptr<byte>(stbi_load(
  //      absolute_path.string().c_str(), &width, &height, &nrComponents, 0));

  img_array_t data;
  if (gamma) {
    data = std::shared_ptr<float>(stbi_loadf(
        absolute_path.string().c_str(), &width, &height, &nrComponents, 0));
  } else {
    data = std::shared_ptr<byte>(stbi_load(absolute_path.string().c_str(),
                                           &width, &height, &nrComponents, 0));
  }

  AssertLog(!data.valueless_by_exception(),
            "stbi load failed! path {} doesn't not exists!", path.string());
  stbi_set_flip_vertically_on_load(false);
  return {data, gamma, nrComponents, height, width};
}
void Texture2D::update_data(const fs::path &path, bool gamma, bool flip) {
#ifndef NDEBUG
  file_ = path;
#endif
  auto res = TextureFromFile(path, gamma, flip);
  auto [internal_format, img_format, img_data_type] = get_internalf_format(res);
  glTextureStorage2D(obj_, 1, internal_format, res.width, res.height);
  if (img_data_type == GL_FLOAT) // float
  {
    glTextureSubImage2D(obj_, 0, 0, 0, res.width, res.height, img_format,
                        img_data_type, std::get<0>(res.data_ptr).get());
  } else {
    glTextureSubImage2D(obj_, 0, 0, 0, res.width, res.height, img_format,
                        img_data_type, std::get<1>(res.data_ptr).get());
  }
  updated_ = true;
}
Texture2D::Texture2D(const fs::path &path, bool gamma, bool flip)
    : Texture(GL_TEXTURE_2D) {
  update_data(path, gamma, flip);
}
Texture2D::Texture2D(int width, int height, GLenum internal_format,
                     GLenum img_format, GLenum img_data_type, const void *data)
    : Texture(GL_TEXTURE_2D) {
  glTextureStorage2D(obj_, 1, internal_format, width, height);
  if (data) {
    glTextureSubImage2D(obj_, 0, 0, 0, width, height, img_format, img_data_type,
                        data);
  }
  updated_ = true;
}
void TextureCube::update_data(const std::vector<fs::path> &paths, bool gamma,
                              bool flip) {
  AssertLog(paths.size() == 6, "Wrong numbers of images {}", paths.size());
#ifndef NDEBUG
  file_ = paths[0];
#endif
  for (int i = 0; i < 6; i++) {
    auto res = TextureFromFile(paths[i], gamma, flip);
    GLenum internal_format = GL_RGB8, img_format = GL_RGB,
           img_data_type = GL_FLOAT;
    if (i == 0) {
      std::tie(internal_format, img_format, img_data_type) =
          get_internalf_format(res);
    }
    glTextureStorage2D(obj_, 1, internal_format, res.width, res.height);
    // An ugly hack here is to use glTextureSubImage3D to remove the binding
    // like  glTextureSubImage3D(cubemap, 0, 0, 0, 0, N, N, 6, GL_RGBA,
    // GL_HALF_FLOAT, cubemap_data.data());

    //        glBindTexture(GL_TEXTURE_CUBE_MAP, obj_);
    //        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0,
    //        res.width, res.height, format, GL_UNSIGNED_BYTE,
    //        res.bytes.get()); glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    // correct DSA way
    if (res.data_ptr.index() == 0) // float
    {
      glTextureSubImage3D(obj_,
                          0, // only 1 level in example
                          0, 0,
                          i, // the offset to desired cubemap face, which
                             // offset goes to which face above
                          res.width, res.height,
                          1, // depth how many faces to set, if this was 3
                             // we'd set 3 cubemap faces at once
                          img_format, img_data_type,
                          std::get<0>(res.data_ptr).get());

    } else {
      glTextureSubImage3D(obj_, 0, 0, 0, i, res.width, res.height, 1,
                          img_format, img_data_type,
                          std::get<1>(res.data_ptr).get());
    }
    updated_ = true;
  }
}

TextureCube::TextureCube(const std::vector<fs::path> &paths, bool gamma,
                         bool flip)
    : Texture(GL_TEXTURE_CUBE_MAP) {
  update_data(paths, gamma, flip);
  glTextureParameteri(obj_, GL_TEXTURE_WRAP_R, wrap_r_);
}
TextureCube::TextureCube(int width, int height, GLenum internal_format,
                         GLenum img_format, GLenum img_data_type,
                         const void *data)
    : Texture(GL_TEXTURE_CUBE_MAP) {

  glTextureStorage2D(obj_, 1, internal_format, width, height);
  if (data) {
    glTextureSubImage3D(obj_, 0, 0, 0,
                        0, // offset: from 0 to store value
                        width, height,
                        6, // faces: we want to set 6 faces at one time
                        img_format, img_data_type,
                        data); // data should be continuous
  }
  updated_ = true;
}
Texture::Texture(GLenum textureType) : obj_(textureType) {
  glTextureParameteri(obj_, GL_TEXTURE_MIN_FILTER, min_filter_);
  glTextureParameteri(obj_, GL_TEXTURE_MAG_FILTER, mag_filter_);
  glTextureParameteri(obj_, GL_TEXTURE_WRAP_S, wrap_s_);
  glTextureParameteri(obj_, GL_TEXTURE_WRAP_T, wrap_t_);
}
void Texture::bind() {
  AssertLog(updated_, "Texture {} has not been updated!", obj_.handle());
  glBindTextureUnit(slot_, obj_);
  bounded_ = true;
  first_bounded = true;
}
void Texture::unbind() {
  AssertLog(bounded_, "Unbind a unbound texture {}!", obj_.handle());
  glBindTextureUnit(slot_, 0);
  bounded_ = false;
}
void Texture::set_wrap_t(GLint value) {
  glTextureParameteri(obj_, GL_TEXTURE_WRAP_T, value);
  wrap_t_ = value;
}
void Texture::set_wrap_s(GLint value) {
  glTextureParameteri(obj_, GL_TEXTURE_WRAP_S, value);
  wrap_s_ = value;
}
void Texture::set_mag_filter(GLint value) {
  glTextureParameteri(obj_, GL_TEXTURE_MAG_FILTER, value);
  mag_filter_ = value;
}
void Texture::set_min_filter(GLint value) {
  glTextureParameteri(obj_, GL_TEXTURE_MIN_FILTER, value);
  min_filter_ = value;
}
