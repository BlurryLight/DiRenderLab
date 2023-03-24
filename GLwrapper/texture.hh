//
// Created by zhong on 2021/4/26.
//

#ifndef DIRENDERLAB_TEXTURE_HH
#define DIRENDERLAB_TEXTURE_HH
#include "../utils/resource_path_searcher.h"
#include "globject.hh"
#include "program.hh"

namespace DRL {
//    enum class TextureType {
//        None,
//        texture2D = GL_TEXTURE_2D,
//        cubemap = GL_TEXTURE_CUBE_MAP
//    };

#ifdef GL_ARB_BINDLESS
class Texture2DARB;
class TextureCubeARB;
#endif

class TextureSampler{
protected:
  SamplerObject obj_;
  GLint min_filter_ = GL_LINEAR;
  GLint mag_filter_ = GL_LINEAR;
  GLint wrap_s_ = GL_REPEAT;
  GLint wrap_t_ = GL_REPEAT;
  GLint wrap_r_ = GL_REPEAT;
public:
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  TextureSampler(TextureSampler &&other) = default;
  TextureSampler&operator=(TextureSampler &&) = default;
  TextureSampler();

  void set_min_filter(GLint value);
  void set_mag_filter(GLint value);
  void set_wrap_s(GLint value);
  void set_wrap_t(GLint value);
  void set_wrap_r(GLint value);

  static const TextureSampler* GetLinearRepeat();
  static const TextureSampler* GetPointRepeat();
  static const TextureSampler* GetLinearClamp();
  static const TextureSampler* GetPointClamp();
};

class Texture {
protected:
  TextureObject obj_;
  bool bound_ = false;
  bool updated_ = false;
  unsigned int slot_ = 0;
  explicit Texture(GLenum textureType);
  int num_mipmaps_ = 1;

public:
  // for debug use
  fs::path file_;

  bool first_bounded = false; // maybe also updated by framebuffer attach
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  GLint min_filter_ = GL_LINEAR;
  GLint mag_filter_ = GL_LINEAR;
  GLint wrap_s_ = GL_CLAMP_TO_EDGE;
  GLint wrap_t_ = GL_CLAMP_TO_EDGE;
  /**
   * generate mipmaps for a specified texture object \n
   * Will throw an warning if the desired number of mipmaps is 1
   */
  void generateMipmap() {
    if (num_mipmaps_ <= 1) {
      spdlog::warn("Texture has only {} mipmap levels!", num_mipmaps_);
    }
    set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    glGenerateTextureMipmap(obj_);
  }
  /**
   * @brief Set the sampler object
   * 
   * @param value 
   */
  void set_sampler(const TextureSampler* value);
  void set_min_filter(GLint value);
  void set_mag_filter(GLint value);
  void set_wrap_s(GLint value);
  void set_wrap_t(GLint value);
  /**
   * set the texture unit(GL_TEXTURE0/GL_TEXTURE1 and etc.)
   * @param value which texture unit will this texture be bound to when bind()
   */
  void set_slot(unsigned int value) { slot_ = value; }
  void bind();
  void unbind();
  bool isBounded() { return bound_; }
  Texture(Texture &&other) = default;
  Texture &operator=(Texture &&) = default;
};
class TextureRect : public Texture {
public:
  TextureRect() : Texture(GL_TEXTURE_RECTANGLE) {}
  TextureRect(int width, int height, int num_mipmaps, GLenum internal_format);
};

class Texture2DMS : public Texture {
public:
  Texture2DMS() : Texture(GL_TEXTURE_2D_MULTISAMPLE) {}
  Texture2DMS(int width, int height, int num_samples, GLenum internal_format);
};
class Texture2D : public Texture {
public:
  Texture2D() : Texture(GL_TEXTURE_2D) {}
  ~Texture2D() {
    AssertWarning(first_bounded || (obj_.handle() == 0),
              "Texture2D {} from file {} is never bounded!", obj_,
              file_.string());
  }
  Texture2D(const fs::path &path, int num_mipmaps, bool gamma, bool flip);

  /**
   * Allocate space for texture and load from data
   * This function doesn't allow nullptr.\n if mipmaps_levels > 1, data will
   * only be filled in level 0
   * @param width
   * @param height
   * @param num_mipmaps
   * @param internal_format GL_RGB8/GL_R16F/GL_RGBA16F and etc. The format how
   * OpenGL store the texture
   * @param img_format GL_RGB/GL_RGBA/GL_R and etc. The format of pixel value in
   * the data
   * @param img_data_type GL_FLOAT/GL_UNSIGNED_BYTES. GL_FLOAT is for HDR
   * texture
   * @param data
   */
  Texture2D(int width, int height, int num_mipmaps, GLenum internal_format,
            GLenum img_format, GLenum img_data_type, const void *data);
  /**
   * Allocate data for texture but don't fill it.
   * @param width
   * @param height
   * @param num_mipmaps
   * @param internal_format
   */
  Texture2D(int width, int height, int num_mipmaps, GLenum internal_format);
  void update_data(const fs::path &path, int num_mipmaps, bool gamma,
                   bool flip);
  Texture2D(Texture2D &&other) = default;
  Texture2D &operator=(Texture2D &&) = default;

  static Texture2D CreateDummyTexture(glm::vec4 color);
};

#ifdef GL_ARB_BINDLESS
class Texture2DARB : public Texture2D {
private:
  // disable some functions
  using Texture2D::bind;
  using Texture2D::unbind;
  using Texture2D::operator unsigned int;
  using Texture2D::handle;
  using Texture2D::operator=;
  using Texture2D::Texture2D;

protected:
  bool residentd_ = false;
  bool first_residentd_ = false;
  GLuint64 ARB_handle_ = 0;

public:
  // for legacy api
  [[nodiscard]] GLuint tex_handle() const { return obj_.handle(); }
  // for bind-less API
  [[nodiscard]] GLuint64 tex_handle_ARB();

  Texture2DARB(const fs::path &path, int num_mipmaps, bool gamma, bool flip);

  Texture2DARB(int width, int height, int num_mipmaps, GLenum internal_format,
               GLenum img_format, GLenum img_data_format, const void *data);
  // glGetTextureHandleARB for an un-updated texture will throw error
  Texture2DARB() = default;

  Texture2DARB(int width, int height, int num_mipmaps, GLenum internal_format);
  ~Texture2DARB();
  Texture2DARB(Texture2DARB &&other) noexcept : Texture2D(std::move(other)) {
    ARB_handle_ = other.ARB_handle_;
    other.ARB_handle_ = 0;
  }
  Texture2DARB(Texture2D &&other) noexcept : Texture2D(std::move(other)) {
    ARB_handle_ = glGetTextureHandleARB(obj_);
  }
  Texture2DARB &operator=(Texture2DARB &&other) noexcept;
  void make_resident();
  void make_non_resident();
  static Texture2DARB CreateDummyTexture(glm::vec4 color);
};
#endif

class TextureCube : public Texture {
public:
  GLint wrap_r_ = GL_REPEAT;
  TextureCube() : Texture(GL_TEXTURE_CUBE_MAP) {
    glTextureParameteri(obj_, GL_TEXTURE_WRAP_R, wrap_r_);
  }
  TextureCube(int width, int height, int num_mipmaps, GLenum internal_format);
  TextureCube(int width, int height, int num_mipmaps, GLenum internal_format,
              GLenum img_format, GLenum img_data_type, const void *data);
  ~TextureCube() {
    AssertLog(first_bounded || (obj_.handle() == 0),
              "TextureCube {} from file {} is never bounded!", obj_,
              file_.string());
  }
  TextureCube(const std::vector<fs::path> &paths, int num_mipmaps, bool gamma,
              bool flip);
  void update_data(const std::vector<fs::path> &paths, int num_mipmaps,
                   bool gamma, bool flip);
  TextureCube(TextureCube &&other) = default;
  TextureCube &operator=(TextureCube &&) = default;
  void set_wrap_r(GLint value) {
    wrap_r_ = value;
    glTextureParameteri(obj_, GL_TEXTURE_WRAP_R, wrap_r_);
  }
};

#ifdef GL_ARB_BINDLESS
class TextureCubeARB : public TextureCube {
private:
  // disable some functions
  using TextureCube::bind;
  using TextureCube::unbind;
  using TextureCube::operator unsigned int;
  using TextureCube::handle;
  using TextureCube::operator=;
  using TextureCube::TextureCube;

protected:
  bool residentd_ = false;
  bool first_residentd_ = false;
  GLuint64 ARB_handle_ = 0;

public:
  // for legacy api
  [[nodiscard]] GLuint tex_handle() const { return obj_.handle(); }
  // for bind-less API
  [[nodiscard]] GLuint64 tex_handle_ARB();
  TextureCubeARB(const std::vector<fs::path> &paths, int num_mipmaps,
                 bool gamma, bool flip)
      : TextureCube(paths, num_mipmaps, gamma, flip) {}

  TextureCubeARB(int width, int height, int num_mipmaps, GLenum internal_format)
      : TextureCube(width, height, num_mipmaps, internal_format) {}
  TextureCubeARB(int width, int height, int num_mipmaps, GLenum internal_format,
                 GLenum img_format, GLenum img_data_type, const void *data)
      : TextureCube(width, height, num_mipmaps, internal_format, img_format,
                    img_data_type, data) {}

  TextureCubeARB() = default;
  ~TextureCubeARB();
  TextureCubeARB(TextureCubeARB &&other) noexcept;
  TextureCubeARB &operator=(TextureCubeARB &&other) noexcept;
  void make_resident();
  void make_non_resident();
};
#endif
} // namespace DRL

#endif // DIRENDERLAB_TEXTURE_HH
