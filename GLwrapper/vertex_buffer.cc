//
// Created by zhong on 2021/4/24.
//

#include "vertex_buffer.hh"

DRL::UniversalBuffer::UniversalBuffer(const void *data, size_t bytes_length,
                                      GLbitfield usage) {
  upload_data(data, bytes_length, usage);
}

void DRL::UniversalBuffer::upload_data(const void *data, size_t bytes_length,
                                       GLbitfield usage) {
  bytes_length_ = bytes_length;
  glNamedBufferStorage(obj_.handle(), static_cast<int>(bytes_length), data,
                       usage);
  if (!allocated_)
    allocated_ = true;
}

void DRL::UniversalBuffer::update_allocated_data(const void *data,
                                                 size_t bytes_length,
                                                 size_t bytes_offset) {
  AssertLog(allocated_, "Buffer must be allocated before sub data!");
  AssertLog((bytes_offset + bytes_length) < bytes_length_,
            "Buffer update data overflow!");
  glNamedBufferSubData(obj_, (int)bytes_offset, (int)bytes_length, data);
}

DRL::ElementBuffer::ElementBuffer(const void *data, size_t bytes_length,
                                  GLbitfield usage)
    : UniversalBuffer(data, bytes_length, usage){};

DRL::VertexBuffer::VertexBuffer(const void *data, size_t bytes_length,
                                GLbitfield usage)
    : UniversalBuffer(data, bytes_length, usage){};

DRL::UniformBuffer::UniformBuffer(const void *data, size_t bytes_length,
                                  GLbitfield usage)
    : UniversalBuffer(data, bytes_length, usage){};

DRL::ShaderStorageBuffer::ShaderStorageBuffer(const void *data, size_t bytes_length,
                                  GLbitfield usage)
    : UniversalBuffer(data, bytes_length, usage){};


void DRL::ShaderStorageBuffer::unbind() { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot_, 0); }

void DRL::ShaderStorageBuffer::bind() { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot_, handle()); }
