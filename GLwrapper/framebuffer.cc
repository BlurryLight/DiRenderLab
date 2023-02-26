//
// Created by zhong on 2021/4/29.
//

#include "framebuffer.hh"
void DRL::Framebuffer::bind() {
  if (!complete_) {
    auto res = glCheckNamedFramebufferStatus(obj_, GL_FRAMEBUFFER);
    AssertLog(res == GL_FRAMEBUFFER_COMPLETE,
              "Framebuffer {} is not complete! Return value is {} ", obj_, res);
    // if here, the framebuffer is complete
    complete_ = true;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, obj_);
  glViewport(0, 0, vwidth_, vheight_);
  if (clear_when_bind) {
    clear();
  }
}
void DRL::Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
void DRL::Framebuffer::set_draw_buffer(
    const std::vector<GLenum> &buffers) {
  this->draw_buffers_.insert(draw_buffers_.end(), buffers.begin(), buffers.end());
  glNamedFramebufferDrawBuffers(obj_, static_cast<int>(buffers.size()),
                                buffers.data());
}
void DRL::Framebuffer::set_read_buffer(GLenum buffer) {
  glNamedFramebufferReadBuffer(obj_, buffer);
}
void DRL::Framebuffer::set_draw_buffer(GLenum buffer) {
  if(std::find(draw_buffers_.begin(),draw_buffers_.end(),buffer) == draw_buffers_.end() )
  {
    draw_buffers_.push_back(buffer);
  }
  glNamedFramebufferDrawBuffer(obj_, buffer);
}

void DRL::Framebuffer::attach_buffer(GLenum attachment_slot,
                                     const DRL::TextureCubePtr &texture_obj,
                                     GLint mipmap_level, GLint layer) {
  attachments_.emplace(texture_obj);
  texture_obj->first_bounded = true;
  glNamedFramebufferTextureLayer(obj_, attachment_slot, *texture_obj,
                                 mipmap_level, layer);
}
void DRL::Framebuffer::attach_buffer(GLenum attachment_slot,
                                     const DRL::Texture2DPtr &texture_obj,
                                     GLint mipmap_level) {
  attachments_.emplace(texture_obj);
  texture_obj->first_bounded = true;
  glNamedFramebufferTexture(obj_, attachment_slot, *texture_obj, mipmap_level);
}
void DRL::Framebuffer::set_viewport(int w, int h) {
  vwidth_ = w;
  vheight_ = h;
}
void DRL::Framebuffer::set_viewport(const DRL::TextureCubePtr &texture_obj,
                                    GLint mipmap_level) {
  glGetTextureLevelParameteriv(*texture_obj, mipmap_level, GL_TEXTURE_HEIGHT,
                               &vheight_);
  glGetTextureLevelParameteriv(*texture_obj, mipmap_level, GL_TEXTURE_WIDTH,
                               &vwidth_);
}
void DRL::Framebuffer::set_viewport(const DRL::Texture2DPtr &texture_obj,
                                    GLint mipmap_level) {
  glGetTextureLevelParameteriv(*texture_obj, mipmap_level, GL_TEXTURE_HEIGHT,
                               &vheight_);
  glGetTextureLevelParameteriv(*texture_obj, mipmap_level, GL_TEXTURE_WIDTH,
                               &vwidth_);
}
DRL::Framebuffer::Framebuffer(GLenum attachment,
                              const DRL::Texture2DPtr &texture_obj,
                              GLint mipmap_level, glm::vec4 clear_color,
                              float clear_depth, bool clear_when_bind)
    : clear_when_bind(clear_when_bind), clear_color_(clear_color),
      clear_depth_(clear_depth) {
  attach_buffer(attachment, texture_obj, mipmap_level);
  set_viewport(texture_obj, mipmap_level);
}
void DRL::Framebuffer::clear() {
  // glClearBuffer allows you to clear the color attachments to different
  // colors, or to only clear certain attachments. glClear clears every color
  // attachment to the color specified by glClearColor.
  for(int i = 0; i < draw_buffers_.size();i++)
  {
    glClearNamedFramebufferfv(obj_, GL_COLOR, i, glm::value_ptr(clear_color_));
  }
  glClearNamedFramebufferfv(obj_, GL_DEPTH, 0, &clear_depth_);
}
