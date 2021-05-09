//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_VERTEX_BUFFER_HH
#define DIRENDERLAB_VERTEX_BUFFER_HH

#include "globject.hh"
namespace DRL {

class VertexBuffer {
protected:
  VertexBufferObject obj_;
  bool allocated_ = false;

public:
  size_t bytes_length_ = 0; // for debug use
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  VertexBuffer(VertexBuffer &&other) = default;
  VertexBuffer &operator=(VertexBuffer &&) = default;
  VertexBuffer() = default;
  VertexBuffer(const void *data, size_t bytes_length, GLbitfield usage) {
    upload_data(data, bytes_length, usage);
  }
  void upload_data(const void *data, size_t bytes_length, GLbitfield usage) {
    bytes_length_ = bytes_length;
    //    glNamedBufferData(obj_, bytes_length, data, usage);
    glNamedBufferStorage(obj_, static_cast<int>(bytes_length), data, usage);
    if (allocated_)
      allocated_ = true;
  }

  void update_allocated_data(const void *data, size_t bytes_length,
                             size_t bytes_offset) {
    AssertLog(allocated_, "VBO must be allocated before sub data!");
    AssertLog((bytes_offset + bytes_length) < bytes_length_,
              "VBO update data overflow!");
    glNamedBufferSubData(obj_, (int)bytes_offset, (int)bytes_length, data);
  }
};

class ElementBuffer : public VertexBuffer {
public:
  ElementBuffer() = default;
  ElementBuffer(const void *data, size_t bytes_length, GLbitfield usage)
      : VertexBuffer(data, bytes_length, usage) {}
  ElementBuffer(ElementBuffer &&other) = default;
  ElementBuffer &operator=(ElementBuffer &&) = default;
};
} // namespace DRL

#endif // DIRENDERLAB_VERTEX_BUFFER_HH
