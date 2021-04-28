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
    class Texture {
    protected:
        TextureObject obj_;
        bool bounded_ = false;
        bool first_bounded = false;
        bool updated_ = false;
        int slot_ = 0;

    public:
        GLenum min_filter_ = GL_NEAREST;
        GLenum mag_filter_ = GL_NEAREST;
        GLenum wrap_s_ = GL_REPEAT;
        GLenum wrap_t_ = GL_REPEAT;
        void generateMipmap() const {
            glGenerateTextureMipmap(obj_);
        }
        Texture() {
            glTextureParameteri(obj_, GL_TEXTURE_MIN_FILTER, min_filter_);
            glTextureParameteri(obj_, GL_TEXTURE_MAG_FILTER, mag_filter_);
            glTextureParameteri(obj_, GL_TEXTURE_WRAP_S, wrap_s_);
            glTextureParameteri(obj_, GL_TEXTURE_WRAP_T, wrap_t_);
        }
        void set_min_filter(GLenum value) {
            glTextureParameteri(obj_, GL_TEXTURE_MIN_FILTER, value);
            min_filter_ = value;
        }
        void set_mag_filter(GLenum value) {
            glTextureParameteri(obj_, GL_TEXTURE_MAG_FILTER, value);
            mag_filter_ = value;
        }
        void set_wrap_s(GLenum value) {
            glTextureParameteri(obj_, GL_TEXTURE_WRAP_S, value);
            wrap_s_ = value;
        }
        void set_wrap_t(GLenum value) {
            glTextureParameteri(obj_, GL_TEXTURE_WRAP_T, value);
            wrap_t_ = value;
        }
        void set_slot(unsigned int value) {
            slot_ = value;
        }
        void bind() {
            AssertLog(updated_, "Texture {} has not been updated!", obj_.handle());
            glBindTextureUnit(slot_, obj_);
            bounded_ = true;
            first_bounded = true;
        }

        void unbind() {
            AssertLog(bounded_, "Unbind a unbound texture {}!", obj_.handle());
            glBindTextureUnit(slot_, 0);
            bounded_ = false;
        }
    };
    class Texture2D : public Texture {
    public:
        Texture2D() = default;
        ~Texture2D() {
            AssertLog(first_bounded || (obj_.handle() == 0), "Texture2D {} is never bounded!", obj_);
        }
        Texture2D(const fs::path &path, bool gamma, bool flip);
        Texture2D(int width, int height, GLenum format, GLenum type, const void *data);
        void update_data(const fs::path &path, bool gamma, bool flip);
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Texture2D(Texture2D &&other) = default;
        Texture2D &operator=(Texture2D &&) = default;
    };
}// namespace DRL


#endif//DIRENDERLAB_TEXTURE_HH
