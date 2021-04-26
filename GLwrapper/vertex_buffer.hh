//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_VERTEX_BUFFER_HH
#define DIRENDERLAB_VERTEX_BUFFER_HH

#include "globject.hh"
namespace DRL {
    enum BufferUsage {
        kStaticDraw = GL_STATIC_DRAW,
        kStaticRead = GL_STATIC_READ,
        kStaticCopy = GL_STATIC_COPY,
        kDynamicDraw = GL_DYNAMIC_DRAW,
        kDynamicRead = GL_DYNAMIC_READ,
        kDynamicCopy = GL_DYNAMIC_COPY,
        kStreamDraw = GL_STREAM_DRAW,
        kStreamRead = GL_STREAM_READ,
        kStreamCopy = GL_STREAM_COPY,
    };
    class VertexBuffer {
    protected:
        VertexBufferObject obj_;

    public:
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        VertexBuffer(VertexBuffer &&other) = default;
        VertexBuffer &operator=(VertexBuffer &&) = default;
        VertexBuffer() = default;
        VertexBuffer(const void *data, size_t bytes_length, BufferUsage usage) {
            upload_data(data, bytes_length, usage);
        }
        void upload_data(const void *data, size_t bytes_length, BufferUsage usage) const {
            glNamedBufferData(obj_, bytes_length, data, usage);
        }
    };

    class ElementBuffer : public VertexBuffer {
    };
}// namespace DRL


#endif//DIRENDERLAB_VERTEX_BUFFER_HH
