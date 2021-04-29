//
// Created by zhong on 2021/4/29.
//

#ifndef DIRENDERLAB_FRAMEBUFFER_HH
#define DIRENDERLAB_FRAMEBUFFER_HH

#include "global.hh"
#include "globject.hh"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
namespace DRL {
    class Framebuffer {
    protected:
        FramebufferObj obj_;
        bool complete_ = false;
        int vheight_ = -1;
        int vwidth_ = -1;

    public:
        glm::vec3 clear_color_ = {};
        glm::vec3 clear_depth_{1.0};
        ~Framebuffer() {
            AssertLog(complete_ || (handle() == 0), "Framebuffer {} is not complete until destroying!", obj_);
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Framebuffer() = default;
        Framebuffer(Framebuffer &&) = default;
        Framebuffer &operator=(Framebuffer &&) = default;
        Framebuffer(GLenum attachment, GLuint texture_obj, GLint mipmap_level, glm::vec3 clear_color = glm::vec3(0.0f),
                    glm::vec3 clear_depth = glm::vec3(1.0f))
            : clear_color_(clear_color), clear_depth_(clear_depth) {
            attach_buffer(attachment, texture_obj, mipmap_level);
            set_viewport(texture_obj, mipmap_level);
        }

        void set_viewport(GLuint texture_obj, GLint mipmap_level) {
            glGetTextureLevelParameteriv(texture_obj, mipmap_level, GL_TEXTURE_HEIGHT, &vheight_);
            glGetTextureLevelParameteriv(texture_obj, mipmap_level, GL_TEXTURE_WIDTH, &vwidth_);
        }
        void set_viewport(int w, int h) {
            vwidth_ = w;
            vheight_ = h;
        }
        void attach_buffer(GLenum attachment, GLuint texture_obj, GLint mipmap_level) {
            glNamedFramebufferTexture(obj_, attachment, texture_obj, mipmap_level);
        }
        void attach_buffer(GLenum attachment, GLuint texture_obj, GLint mipmap_level, GLint layer) {
            glNamedFramebufferTextureLayer(obj_, attachment, texture_obj, mipmap_level, layer);
        }
        void set_draw_buffer(GLenum buffer) const {
            glNamedFramebufferDrawBuffer(obj_, buffer);
        }
        void set_read_buffer(GLenum buffer) const {
            glNamedFramebufferReadBuffer(obj_, buffer);
        }
        void set_draw_buffer(const std::vector<GLenum> &buffers) const {
            glNamedFramebufferDrawBuffers(obj_, buffers.size(), buffers.data());
        }
        void bind() {
            if (!complete_) {
                auto res = glCheckNamedFramebufferStatus(obj_, GL_FRAMEBUFFER);
                AssertLog(res == GL_FRAMEBUFFER_COMPLETE,
                          "Framebuffer {} is not complete! Return value is {} ", obj_, res);
                // if here, the framebuffer is complete
                complete_ = true;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, obj_);
            glViewport(0, 0, vwidth_, vheight_);
            glClearNamedFramebufferfv(obj_, GL_COLOR, 0, glm::value_ptr(clear_color_));
            glClearNamedFramebufferfv(obj_, GL_DEPTH, 0, glm::value_ptr(clear_depth_));
        }
        void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    };

}// namespace DRL


#endif//DIRENDERLAB_FRAMEBUFFER_HH