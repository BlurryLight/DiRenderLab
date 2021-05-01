//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_VERTEX_ARRAY_HH
#define DIRENDERLAB_VERTEX_ARRAY_HH

#include "program.hh"
#include "vertex_buffer.hh"
#include <optional>
#include <vector>
namespace DRL {
    struct AttributeInfo {
        GLuint location;
        GLenum type;
        GLint count;
        GLint offset;
    };

    class VertexArray {
    protected:
        VertexArrayObject obj_;
        //        std::optional<std::vector<IndexInfo>> indices_;
        bool ebo_ = false;
        bool bounded_ = false;
        bool changed_ = false;// attribute changed since last update_bind
        std::vector<AttributeInfo> attribs_;

    public:
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        VertexArray(VertexArray &&other) = default;
        VertexArray &operator=(VertexArray &&) = default;
        VertexArray() = default;
        void lazy_bind_attrib(GLuint location, GLenum type, GLint count, GLint num_offset) {
            attribs_.push_back({location, type, count, num_offset});
            if (!changed_) changed_ = true;
        }
        void lazy_bind_attrib(const Program &prog, const std::string &name, GLenum type, GLint count, GLint num_offset) {
            AssertLog(prog.linked(), "Program {} should be linked when looking up attribute location!", prog.handle());
            int pos = glGetAttribLocation(prog, name.c_str());
            AssertLog(pos != -1, "Program {} doesn't have attribute {} ", prog.handle(), name);
            attribs_.push_back({static_cast<unsigned int>(pos), type, count, num_offset});
        }

        //will keep state
        void update_bind(const VertexBuffer &vbo, GLuint first_element_offset,GLint num_offset,size_t elem_size) {
            AssertLog(!attribs_.empty(), "VAO: {} Nothing to update: Attributes is empty!", obj_);
            for (const auto &attrib : attribs_) {
                glEnableVertexArrayAttrib(obj_, attrib.location);
                glVertexArrayAttribFormat(obj_, attrib.location, attrib.count, attrib.type, GL_FALSE, elem_size * attrib.offset);
                //https://www.reddit.com/r/opengl/comments/eg9b0a/what_is_the_bindingindex_2nd_arg_in/
                glVertexArrayAttribBinding(obj_, attrib.location, 0);
            }
            // now we set the vbo
            glVertexArrayVertexBuffer(obj_, 0, vbo, first_element_offset, elem_size * num_offset);
            changed_ = false;
        }
        //TODO: vbo,ebo should transfer its ownership to VertexArray
        void update_bind(const VertexBuffer &vbo, const ElementBuffer &ebo, GLuint first_element_offset, GLint num_offset,size_t elem_size) {
            //            glBindVertexArray(obj_);
            AssertLog(!attribs_.empty(), "VAO: {} Nothing to update: Attributes is empty!", obj_);
            ebo_ = true;
            for (const auto &attrib : attribs_) {
                glEnableVertexArrayAttrib(obj_, attrib.location);
                glVertexArrayAttribFormat(obj_, attrib.location, attrib.count, attrib.type, GL_FALSE, elem_size * attrib.offset);
                glVertexArrayAttribBinding(obj_, attrib.location, 0);
            }
            // now we set the vbo
            glVertexArrayVertexBuffer(obj_, 0, vbo, first_element_offset, elem_size * num_offset);
            glVertexArrayElementBuffer(obj_, ebo);
            changed_ = false;
        }
        void bind() {
            AssertLog(!changed_, "Some attributes of VAO {} have yet been updated. Call update_bind() firstly!", obj_.handle());
            glBindVertexArray(obj_);
            bounded_ = true;
        }
        void draw(GLenum mode, GLint first_index, GLsizei indices_nums) {
            AssertLog(bounded_, "VAO {} hasn't been bounded before drawing!", obj_.handle());
            AssertLog(!ebo_, "VAO {} has ebo, wrong version of draw() is called!!", obj_.handle());
            glDrawArrays(mode, first_index, indices_nums);
        }

        void draw(GLenum mode, GLsizei indices_nums, GLenum indices_type, void *ebo_offset = nullptr) {
            AssertLog(bounded_, "VAO {} hasn't been bounded before drawing!", obj_.handle());
            AssertLog(ebo_, "VAO {} has not ebo, wrong version of draw() is called!!", obj_.handle());
            glDrawElements(mode, indices_nums, indices_type, ebo_offset);
        }
        void unbind() {
            glBindVertexArray(0);
            bounded_ = false;
        }
    };
}// namespace DRL

#endif//DIRENDERLAB_VERTEX_ARRAY_HH
