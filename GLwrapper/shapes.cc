//
// Created by zhong on 2021/4/26.
//

#include "shapes.hh"
#include "glad/glad.h"
#include "vertex_array.hh"
using namespace DRL;


void DRL::renderCube() {
  glBindVertexArray(GetCubeVAO());
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}
void DRL::renderQuad() {
  static unsigned int quadVAO = 0;
  static unsigned int quadVBO = 0;
  if (quadVAO == 0) {

    float quadVertices[] = {
        // positions            // normals         // texcoords
        1.0f,  -0.5f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -0.5f, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

        1.0f,  -0.5f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f,  -0.5f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f};
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
  }
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void DRL::renderSphere() {
  static VertexArray *sphereVAO;
  static int sphereIndexSize = 0;
  static VboPtr vbo = nullptr;
  static EboPtr ebo = nullptr;
  if (sphereVAO == nullptr) {
    sphereVAO = new VertexArray();
    vbo = std::make_shared<VertexBuffer>();
    ebo = std::make_shared<ElementBuffer>();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
      for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
        float xSegment = (float)x / (float)X_SEGMENTS;
        float ySegment = (float)y / (float)Y_SEGMENTS;
        float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
        float yPos = std::cos(ySegment * PI);
        float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

        positions.push_back(glm::vec3(xPos, yPos, zPos));
        uv.push_back(glm::vec2(xSegment, ySegment));
        normals.push_back(glm::vec3(xPos, yPos, zPos));
      }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
      if (!oddRow) // even rows: y == 0, y == 2; and so on
      {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
          indices.push_back(y * (X_SEGMENTS + 1) + x);
          indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
        }
      } else {
        for (int x = X_SEGMENTS; x >= 0; --x) {
          indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
          indices.push_back(y * (X_SEGMENTS + 1) + x);
        }
      }
      oddRow = !oddRow;
    }
    sphereIndexSize = (int)indices.size();

    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i) {
      data.push_back(positions[i].x);
      data.push_back(positions[i].y);
      data.push_back(positions[i].z);
      if (!normals.empty()) {
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
      }
      if (!uv.empty()) {
        data.push_back(uv[i].x);
        data.push_back(uv[i].y);
      }
    }
    vbo->upload_data(data.data(), data.size() * sizeof(float),
                     GL_DYNAMIC_STORAGE_BIT);
    ebo->upload_data(indices.data(), indices.size() * sizeof(float),
                     GL_DYNAMIC_STORAGE_BIT);
    int stride = (3 + 2 + 3);
    sphereVAO->lazy_bind_attrib(0, GL_FLOAT, 3, 0);
    sphereVAO->lazy_bind_attrib(1, GL_FLOAT, 3, 3);
    sphereVAO->lazy_bind_attrib(2, GL_FLOAT, 2, 6);
    sphereVAO->update_bind(vbo, ebo, 0, stride, sizeof(float));
  }

  bind_guard<VertexArray> gd(*sphereVAO);
  sphereVAO->draw(GL_TRIANGLE_STRIP, sphereIndexSize, GL_UNSIGNED_INT, nullptr);
}
unsigned int DRL::GetCubeVAO() {
  static unsigned int cubeVAO = 0;
  static unsigned int cubeVBO = 0;
  // initialize (if necessary)
  if (cubeVAO == 0) {
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
        1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
        // front face
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
        -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
        // right face
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
        1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-right
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
        1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
        1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
        -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
        -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
    };
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // link vertex attributes
    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
  return cubeVAO;
}
void DRL::renderScreenQuad() {
  static DRL::VertexArray *quadVAO = nullptr;
  if (!quadVAO) {
    quadVAO = new DRL::VertexArray();
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    auto quad_vbo_ptr = std::make_shared<DRL::VertexBuffer>(
        quadVertices, sizeof quadVertices, GL_DYNAMIC_STORAGE_BIT);
    quadVAO->lazy_bind_attrib(0, GL_FLOAT, 3, 0);
    quadVAO->lazy_bind_attrib(1, GL_FLOAT, 2, 3);
    quadVAO->update_bind(quad_vbo_ptr, 0, 5, sizeof(float));
  }
  DRL::bind_guard gd(*quadVAO);
  quadVAO->draw(GL_TRIANGLE_STRIP, 0, 4);
}
