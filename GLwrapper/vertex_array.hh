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
using VboPtr = std::shared_ptr<VertexBuffer>;
using EboPtr = std::shared_ptr<VertexBuffer>;

class VertexArray {
protected:
  VertexArrayObject obj_;
  //        std::optional<std::vector<IndexInfo>> indices_;
  bool bound_ = false;
  bool changed_ = false; // attribute changed since last update_bind
  std::vector<AttributeInfo> attribs_;
  std::vector<VboPtr> vbos_;
  EboPtr ebo_ = nullptr; // a vao can only bind an ebo

public:
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  VertexArray(VertexArray &&other) = default;
  VertexArray &operator=(VertexArray &&) = default;
  VertexArray() = default;
  void lazy_bind_attrib(GLuint location, GLenum type, GLint count,
                        GLint num_offset) {
    attribs_.push_back({location, type, count, num_offset});
    if (!changed_)
      changed_ = true;
  }
  void lazy_bind_attrib(const Program &program, const std::string &name,
                        GLenum type, GLint count, GLint num_offset) {
    AssertLog(program.linked(),
              "Program {} should be linked when looking up attribute location!",
              program.handle());
    int pos = glGetAttribLocation(program, name.c_str());
    AssertLog(pos != -1, "Program {} doesn't have attribute {} ",
              program.handle(), name);
    attribs_.push_back(
        {static_cast<unsigned int>(pos), type, count, num_offset});
  }

  void update_bind_slot(const VboPtr &vbo, GLuint vbo_slot,
                        GLuint first_element_offset, GLint num_offset,
                        size_t elem_size) {
    AssertLog(!attribs_.empty(),
              "VAO: {} Nothing to update: Attributes is empty!", obj_);
    for (const auto &attrib : attribs_) {
      glEnableVertexArrayAttrib(obj_, attrib.location);
      glVertexArrayAttribFormat(obj_, attrib.location, attrib.count,
                                attrib.type, GL_FALSE,
                                static_cast<int>(elem_size) * attrib.offset);
      // https://www.reddit.com/r/opengl/comments/eg9b0a/what_is_the_bindingindex_2nd_arg_in/
      glVertexArrayAttribBinding(obj_, attrib.location, vbo_slot);
    }
    // now we set the vbo
    glVertexArrayVertexBuffer(obj_, vbo_slot, *vbo, first_element_offset,
                              static_cast<int>(elem_size) * num_offset);
    vbos_.push_back(vbo);
    changed_ = false;
  }
  void update_bind_slot(const VboPtr &vbo, GLuint vbo_slot, const EboPtr &ebo,
                        GLuint first_element_offset, GLint num_offset,
                        size_t elem_size) {
    update_bind_slot(vbo, vbo_slot, first_element_offset, num_offset,
                     elem_size);
    glVertexArrayElementBuffer(obj_, *ebo);
    ebo_ = ebo;
  }

  // will keep state
  void update_bind(const VboPtr &vbo, GLuint first_element_offset,
                   GLint num_offset, size_t elem_size) {
    update_bind_slot(vbo, 0, first_element_offset, num_offset, elem_size);
  }

  // vbo,ebo should have longer lifetime than the VertexArray
  void update_bind(const VboPtr &vbo, const EboPtr &ebo,
                   GLuint first_element_offset, GLint num_offset,
                   size_t elem_size) {
    update_bind_slot(vbo, 0, ebo, first_element_offset, num_offset, elem_size);
  }
  void bind() {
    AssertLog(!changed_,
              "Some attributes of VAO {} have yet been updated. Call "
              "update_bind() firstly!",
              obj_.handle());
    glBindVertexArray(obj_);
    bound_ = true;
  }
  void draw(GLenum mode, GLint first_index, GLsizei indices_nums) {
    AssertLog(bound_, "VAO {} hasn't been bounded before drawing!",
              obj_.handle());
    AssertLog((ebo_ == nullptr),
              "VAO {} has ebo, wrong version of draw() is called!!",
              obj_.handle());
    glDrawArrays(mode, first_index, indices_nums);
  }

  void draw(GLenum mode, GLsizei indices_nums, GLenum indices_type,
            void *ebo_offset = nullptr) {
    AssertLog(bound_, "VAO {} hasn't been bounded before drawing!",
              obj_.handle());
    AssertLog((bool)ebo_,
              "VAO {} has not ebo, wrong version of draw() is called!!",
              obj_.handle());
    glDrawElements(mode, indices_nums, indices_type, ebo_offset);
  }
  void unbind() {
    glBindVertexArray(0);
    bound_ = false;
  }
};
} // namespace DRL

#endif // DIRENDERLAB_VERTEX_ARRAY_HH
