#include "text_overlay.hh"
using namespace DRL;
DRL::TextOverlay::TextOverlay(int width,int height) : mBufferWidth(width),mBufferHeight(height){
    mTextBufferGPU = std::make_unique<DRL::Texture2D>(width,height,1,GL_R8UI);
    mPathSearcher.add_path(decltype(mPathSearcher)::root_path / "resources" / "shaders" / "Textoverlay");
    mPathSearcher.add_path(decltype(mPathSearcher)::root_path / "resources" / "textures" / "bitmaps");
    mBitMapShader =
        DRL::make_program(mPathSearcher.find_path("ScreenOverlay.vs"),
                          mPathSearcher.find_path("BitMapFont.fs"));
    mTextBufferCPU.resize(height * width,0);
    glTextureSubImage2D(mTextBufferGPU->handle(),0,0,0,mBufferWidth,mBufferHeight,GL_RED_INTEGER,GL_UNSIGNED_BYTE,mTextBufferCPU.data());

    mBitMap = std::make_unique<DRL::Texture2D>(
        mPathSearcher.find_path("cp437_9x16.png"), 1, false, /*flip*/false);
    mBitMap->set_slot(1);
}

void DRL::TextOverlay::draw() {
    glDepthMask(false);
    glStencilMask(0x00);
    DRL::bind_guard gd(mTextBufferGPU,mBitMap,mVAO,mBitMapShader);
    if(mDirty)
    {
        glTextureSubImage2D(mTextBufferGPU->handle(),0,0,0,mBufferWidth,mBufferHeight,GL_RED_INTEGER,GL_UNSIGNED_BYTE,mTextBufferCPU.data());
        mDirty = false;
    }
    mBitMapShader.set_uniform("BitMapInfo", glm::ivec4(9,16,32,mScale));
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glDepthMask(true);
    glStencilMask(0xff);
}

void DRL::TextOverlay::drawText(const std::string & str) {
    mDirty = true;
    auto it = str.cbegin();
    decltype(mTextBufferCPU)::iterator dst = mTextBufferCPU.begin() + CursorY  * mBufferWidth + CursorX;
    char c = '\0';
    while(it != str.cend() && *it != '\0' && dst != mTextBufferCPU.end())
    {
        c = *it++;
        if(c == '\n')
        {
            CursorX = 0;
            CursorY++;
            if(CursorY >= mBufferHeight)
            {
                CursorY--;
                scroll(1);
            }
            dst = mTextBufferCPU.begin() + CursorY  * mBufferWidth + CursorX;
        }
        else
        {
            *dst = c;
            dst++;
            CursorX++;
            if(CursorX >= mBufferWidth)
            {
                CursorY++;
                CursorX = 0;
                if(CursorY >= mBufferHeight)
                {
                    CursorY--;
                    scroll(1);
                }
                dst = mTextBufferCPU.begin() + CursorY  * mBufferWidth + CursorX;
            }
        }

    }
}
void DRL::TextOverlay::scroll(int lines) {
    std::vector<unsigned char> tmp(mTextBufferCPU.size(),0);
    std::copy(mTextBufferCPU.begin() + lines * mBufferWidth,mTextBufferCPU.end(),tmp.begin());
    mTextBufferCPU = std::move(tmp);
    mDirty = true;
}
void DRL::TextOverlay::MoveCursor(int x,int y)
{
    CursorX = std::clamp(x, 0, mBufferWidth);
    CursorY = std::clamp(y,0,mBufferHeight);
}
void DRL::TextOverlay::drawText(const std::string & str,int x,int y) {
    MoveCursor(x,y);
    drawText(str);
    /*
    int offset = y * mBufferWidth + x;
    int num_chars = str.size();
    int buffer_size = this->mBufferWidth * this->mBufferHeight;
    if(offset + str.size() >= buffer_size)
    {
        num_chars =  buffer_size - offset;
    }
    std::copy_n(str.begin(),num_chars,mTextBufferCPU.begin() + offset);
    mDirty = true;
    */
}

void DRL::TextOverlay::clear() {
    // mTextBufferCPU.clear();
    std::fill(mTextBufferCPU.begin(),mTextBufferCPU.end(),0);
    mDirty = true;
    CursorX = 0;
    CursorY = 0;
}
