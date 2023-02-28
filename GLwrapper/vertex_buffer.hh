//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_VERTEX_BUFFER_HH
#define DIRENDERLAB_VERTEX_BUFFER_HH

#include "globject.hh"
namespace DRL {

class UniversalBuffer {
protected:
  UniversalBufferObject obj_;
  bool allocated_ = false;

  UniversalBuffer() = default;
  UniversalBuffer(const void *data, size_t bytes_length, GLbitfield usage);
public:
  size_t bytes_length_ = 0; // for debug use
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  UniversalBuffer(UniversalBuffer &&other) = default;
  UniversalBuffer &operator=(UniversalBuffer &&) = default;
  void upload_data(const void *data, size_t bytes_length, GLbitfield usage);

  void update_allocated_data(const void *data, size_t bytes_length,
                             size_t bytes_offset);
};

class VertexBuffer : public UniversalBuffer{
public:
  VertexBuffer(VertexBuffer &&other) = default;
  VertexBuffer &operator=(VertexBuffer &&) = default;
  VertexBuffer() = default;
  VertexBuffer(const void *data, size_t bytes_length, GLbitfield usage);
};

class ElementBuffer : public UniversalBuffer{
public:
  ElementBuffer() = default;
  ElementBuffer(const void *data, size_t bytes_length, GLbitfield usage);
  ElementBuffer(ElementBuffer &&other) = default;
  ElementBuffer &operator=(ElementBuffer &&) = default;
};

class UniformBuffer: public UniversalBuffer{
private:
  // binding to index slot
  // shader: layout (binding = 0)
  int slot_ = 0;
public:
  UniformBuffer() = default;
  UniformBuffer(const void *data, size_t bytes_length, GLbitfield usage);
  UniformBuffer(UniformBuffer &&other) = default;
  UniformBuffer &operator=(UniformBuffer &&) = default;

  int get_slot() const { return slot_; }
  void set_slot(int slot) { slot_ = slot; }
  void bind() { glBindBufferBase(GL_UNIFORM_BUFFER, slot_, handle()); }
  void unbind() { glBindBufferBase(GL_UNIFORM_BUFFER, slot_, 0); }
};
} // namespace DRL

#endif // DIRENDERLAB_VERTEX_BUFFER_HH
