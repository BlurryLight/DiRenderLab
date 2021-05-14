# DiRenderLab

Windows MSVC2019 & Ubuntu latest:

[![CI](https://github.com/BlurryLight/DiRenderLab/actions/workflows/build.yml/badge.svg)](https://github.com/BlurryLight/DiRenderLab/actions/workflows/build.yml)

A lab for real-time rendering based on OpenGL4.5 with DSA.

The Framework is originally based on LearnOpenGL.com. Some code and resources are from LearnOpenGL.com.

# Compile & run

- OpenGL 4.5+ and ARB_bindless_texture are needed.
 
  **Known issue**: Intel GPU may **NOT** support the bindless extension.
- Mainly developed and tested on Nvidia GTX1050TI. Some code are also tested on AMD GPU.
- Compile: go for [build.yaml](.github/workflows/build.yml) for details.

# Milestones

- SSAO
  ![ssao](images/SSAO.jpg)

- Shadows(ShadowMap,PCF,PCSS)
  ![shadows](images/shadowmap.jpg)

- PBR(No IBL)
  ![pbr](images/pbr_no_IBL.jpg)

- PBR(IBL)

  note: Code, especially the shader code, is copied from [LearnOpenGL](https://learnopengl.com/PBR/IBL/Specular-IBL).

  ![pbr](images/pbr_IBL.jpg)

- Weighted Blended OIT

  note: Code, especially the shader code, is copied from [LearnOpenGL](https://learnopengl.com)

  ![oit](images/weighted_blended_oit.jpg)

