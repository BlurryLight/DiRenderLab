// copyright from sb7code (OpenGL Superbible 7)
// modified by BlurryLight
#pragma once
#include "GLwrapper/glsupport.hh"
namespace DRL {
    class TextOverlay
    {
        private:
        int CursorX= 0;
        int CursorY= 0;
        std::unique_ptr<DRL::Texture2D> mBitMap = nullptr;
        std::unique_ptr<DRL::Texture2D> mTextBufferGPU = nullptr;
        std::vector<unsigned char> mTextBufferCPU; 
        DRL::Program mBitMapShader;
        DRL::ResourcePathSearcher mPathSearcher;
        DRL::VertexArray mVAO;
        int mBufferWidth = 80;
        int mBufferHeight = 32;
        bool mDirty = true;


        public:
        int mScale = 1; // only support int scale
        TextOverlay(int width = 80,int height = 32);
        void draw();
        void drawText(const std::string & str,int x,int y);
        void drawText(const std::string & str);
        void scroll(int lines);
        void MoveCursor(int x,int y);
        void clear();
    };

}