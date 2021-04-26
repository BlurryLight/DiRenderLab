//
// Created by zhong on 2021/4/26.
//

#include "texture.hh"
#include "../third_party/stb_image.h"
#include "global.hh"
#include <memory>
#include <utility>
using namespace DRL;
using byte = uint8_t;
struct LoadReturnType {
    std::shared_ptr<byte> bytes;
    int nchannels;
    int height;
    int width;
};
static LoadReturnType TextureFromFile(const fs::path &path, bool gamma, bool flip) {
    if (flip) {
        stbi_set_flip_vertically_on_load(true);
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    auto absolute_path = fs::absolute(path);
    auto data = std::shared_ptr<byte>(stbi_load(absolute_path.string().c_str(), &width, &height, &nrComponents, 0));
    AssertLog(data.get(), "stbi load failed! path {} doesn't not exists!", path.string());
    stbi_set_flip_vertically_on_load(false);

    return {data, nrComponents, height, width};
}
void Texture2D::update_data(const fs::path &path, bool gamma, bool flip) {
    auto res = TextureFromFile(path, gamma, flip);
    GLenum internal_format = GL_RGB8, format = GL_RGB;
    switch (res.nchannels) {
        case 1:
            internal_format = GL_R8;
            format = GL_RED;
            break;
        case 2:
            internal_format = GL_RG8;
            format = GL_RG;
            break;
        case 3:
            internal_format = GL_RGB8;
            format = GL_RGB;
            break;
        case 4:
            internal_format = GL_RGBA8;
            format = GL_RGBA;
            break;
    }

    glTextureStorage2D(obj_, 1, internal_format, res.width, res.height);
    glTextureSubImage2D(obj_, 0, 0, 0, res.width, res.height, format, GL_UNSIGNED_BYTE, res.bytes.get());
    updated_ = true;
}
Texture2D::Texture2D(const fs::path &path, bool gamma, bool flip) {
    update_data(path, gamma, flip);
}
Texture2D::Texture2D(int width, int height, GLenum format, GLenum type, const void *data) {
    glTextureStorage2D(obj_, 1, format, width, height);
    updated_ = true;
}
