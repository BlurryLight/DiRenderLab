//
// Created by zhong on 2021/4/29.
//

#ifndef DIRENDERLAB_FRAMEBUFFER_HH
#define DIRENDERLAB_FRAMEBUFFER_HH

#include "global.hh"
#include "globject.hh"
#include "texture.hh"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
namespace DRL {
// renderbuffer not implemented
//    class Renderbuffer;
using Texture2DPtr = std::shared_ptr<Texture2D>;
using Texture2DMSPtr = std::shared_ptr<Texture2DMS>;
using TextureCubePtr = std::shared_ptr<TextureCube>;
//    using RenderbufferPtr = std::shared_ptr<Renderbuffer>;
class Framebuffer {
protected:
  FramebufferObj obj_;
  bool complete_ = false;
  int vheight_ = -1;
  int vwidth_ = -1;
  using attachable_obj =
      std::variant<Texture2DPtr, TextureCubePtr,Texture2DMSPtr /*,RenderbufferPtr*/>;
  std::set<attachable_obj> attachments_;

  std::vector<GLenum> draw_buffers_;
public:
  bool clear_when_bind = true;
  glm::vec4 clear_color_ = {};
  float clear_depth_{1.0};
  ~Framebuffer() {
    AssertLog(complete_ || (handle() == 0),
              "Framebuffer {} is not complete until destroying!", obj_);
  }
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  Framebuffer() = default;
  Framebuffer(Framebuffer &&) = default;
  Framebuffer &operator=(Framebuffer &&) = default;
  /**
   * construct Framebuffer object from a texture obj
   * @param attachment GL_DEPTH_ATTACHMENT or COLOR_ATTACHMENT, decided by the
   * use of texture_obj
   * @param texture_obj the attachment to FBO
   * @param mipmap_level mipmap level of the texture
   * @param clear_color RGBA value to write when clear FBO. If multiple
   * color attachment attached, all of them will be cleared.
   * @param clear_depth float Depth value to write when clear FBO
   * @param clear_when_bind Whether to clear FBO when bind()
   */
  Framebuffer(GLenum attachment, const Texture2DPtr &texture_obj,
              GLint mipmap_level, glm::vec4 clear_color = glm::vec4(0.0f),
              float clear_depth = 1.0f, bool clear_when_bind = true);

  /**
   * set framebuffer viewport weight/height by properties from the nth mipmap of
   * the texture
   * @param texture_obj
   * @param mipmap_level
   */
  void set_viewport(const Texture2DPtr &texture_obj, GLint mipmap_level);

  void set_viewport(const TextureCubePtr &texture_obj, GLint mipmap_level);
  /**
   *
   * set framebuffer viewport weight/height by weight and height
   * @param w weight pixels
   * @param h  height pixels
   */
  void set_viewport(int w, int h);
  void attach_buffer(GLenum attachment_slot, const Texture2DPtr &texture_obj,
                     GLint mipmap_level);
  void attach_buffer(GLenum attachment_slot, const Texture2DMSPtr &texture_obj);
  // currently doesn't support renderbuffer object
  //        void attach_buffer(GLenum attachment_slot, const RenderbufferPtr
  //        &renderbuf_obj, GLint mipmap_level) {
  //            attachments_.push_back(texture_obj);
  //            glNamedFramebufferRenderbuffer(obj_,attachment_slot,
  //            GL_RENDERBUFFER,*renderbuf_obj);
  //        }
  void attach_buffer(GLenum attachment_slot, const TextureCubePtr &texture_obj,
                     GLint mipmap_level, GLint layer);
  /**
   * specify which color buffers are to be drawn into
   * For default framebuffer, the argument specifies up to four color buffers to
   * be drawn into.
   * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawBuffer.xhtml\n
   * GL_NONE: No colorbuffers will be written\n
   * GL_FRONT/BACK(_LEFT/RIGHT): if texture is cubemap they are valid\n
   * GL_COLOR_ATTACHMENT$m$: valid for MRT\n
   * @param buffer
   */
  void set_draw_buffer(GLenum buffer);
  /**
   * same as set_draw_buffer()
   * @param buffer
   */
  void set_read_buffer(GLenum buffer);
  void set_draw_buffer(const std::vector<GLenum> &buffers);
  void bind();
  void unbind();
  /**
   *
   * we only clear first color buffer
   * for mrt the user must other colors manually
   * since we don't track how many color buffers are attached to FBO
   */
  void clear();
};

} // namespace DRL

#endif // DIRENDERLAB_FRAMEBUFFER_HH
