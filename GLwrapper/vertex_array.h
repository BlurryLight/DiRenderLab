//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_VERTEX_ARRAY_H
#define DIRENDERLAB_VERTEX_ARRAY_H

#include "program.hh"
#include "vertex_buffer.hh"
#include <vector>
namespace DRL {
    struct AttributeInfo {
        GLuint location;
        GLuint type;
        GLint count;
        intptr_t offset;
        //        Gluint stride
    };
    class VertexArray {
    protected:
        VertexArrayObject obj_;
        bool bounded_ = false;
        bool changed_ = false;// attribute changed since last update_bind
        std::vector<AttributeInfo> attribs_;

    public:
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        VertexArray(VertexArray &&other) = default;
        VertexArray &operator=(VertexArray &&) = default;
        VertexArray() = default;
        void lazy_bind_attrib(GLuint location, GLenum type, GLint count, intptr_t offset) {
            attribs_.push_back({location, type, count, offset});
            if (!changed_) changed_ = true;
        }
        void lazy_bind_attrib(const Program &prog, const std::string &name, GLenum type, GLint count, intptr_t offset) {
            AssertLog(prog.linked(), "Program {} should be linked when looking up attribute location!", prog.handle());
            int pos = glGetAttribLocation(prog, name.c_str());
            AssertLog(pos != -1, "Program {} doesn't have attribute {} ", prog.handle(), name);
            attribs_.push_back({static_cast<unsigned int>(pos), type, count, offset});
        }

        //will keep state
        void update_bind(const VertexBuffer &vbo, GLuint first_element_offset, GLint element_stride) {
            //            glBindVertexArray(obj_);
            for (const auto &attrib : attribs_) {
                glEnableVertexArrayAttrib(obj_, attrib.location);
                glVertexArrayAttribFormat(obj_, attrib.location, attrib.count, attrib.type, GL_FALSE, sizeof(attrib.type) * attrib.offset);
                //https://www.reddit.com/r/opengl/comments/eg9b0a/what_is_the_bindingindex_2nd_arg_in/
                glVertexArrayAttribBinding(obj_, attrib.location, 0);
            }
            // now we set the vbo
            glVertexArrayVertexBuffer(obj_, 0, vbo, first_element_offset, element_stride);
            //            glBindVertexArray(0);
            changed_ = false;
        }
        void bind() {
            AssertLog(!changed_, "Some attributes of VAO {} have yet been updated. Call update_bind() firstly!", obj_.handle());
            glBindVertexArray(obj_);
            bounded_ = true;
        }
        void draw(GLenum mode, GLint first_index, GLsizei indices_nums) {
            AssertLog(bounded_, "VAO {} hasn't been bounded before drawing!", obj_.handle());
            glDrawArrays(mode, first_index, indices_nums);
        }
        void unbind() {
            glBindVertexArray(0);
            bounded_ = false;
        }
    };
}// namespace DRL

#endif//DIRENDERLAB_VERTEX_ARRAY_H
