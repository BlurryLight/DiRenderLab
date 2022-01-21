#pragma once



#if defined _GNUC_ || defined _CLANG_CL || defined __clang__
// lmao: Clang-cl will pretend it is MSVC, but actually it accepts gcc #pragma
// flags.
#pragma GCC diagnostic push // gcc way to suppress -Wpgrama-pack
#pragma GCC diagnostic ignored "-Wpragma-pack"
#elif _MSC_VER
#pragma warning(push, 0) // MSVC way to suppress external headers warning
#endif
//#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <random>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#ifdef _GNUC_
#pragma GCC diagnostic pop
#elif _MSC_VER
#pragma warning(pop)
#endif

#include "program.hh"
#include "shader.hh"
#include "texture.hh"
#include "vertex_array.hh"
#include <GLFW/glfw3.h>
namespace DRL {
// Defines several possible options for camera movement. Used as abstraction to
// stay away from window-system specific input methods
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// Default camera values
constexpr inline float YAW = -90.0f;
constexpr inline float PITCH = 0.0f;
constexpr inline float SPEED = 2.5f;
constexpr inline float SENSITIVITY = 0.1f;
constexpr inline float ZOOM = 45.0f;

float get_random_float(float min = 0.0f, float max = 1.0f);

// An abstract camera class that processes input and calculates the
// corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera {
public:
  // Camera Attributes
  glm::vec3 Position{};
  glm::vec3 Front;
  glm::vec3 Up{};
  glm::vec3 Right{};
  glm::vec3 WorldUp{};
  // Euler Angles
  float Yaw;
  float Pitch;
  // Camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;

  // Constructor with vectors
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
         float pitch = PITCH);
  // Constructor with scalar values
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
         float yaw, float pitch);

  // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
  }

  // Processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void ProcessKeyboard(Camera_Movement direction, float deltaTime);

  // Processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void ProcessMouseMovement(float xoffset, float yoffset,
                            GLboolean constrainPitch = true);

  // Processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis
  void ProcessMouseScroll(float yoffset);

private:
  // Calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors();
};

namespace details {
struct Vertex {
  // position
  glm::vec3 Position;
  // normal
  glm::vec3 Normal;
  // texCoords
  glm::vec2 TexCoords;
  // tangent
  glm::vec3 Tangent;
  // bitangent
  glm::vec3 Bitangent;
};

struct Texture {
  std::shared_ptr<DRL::Texture2D> tex_ptr;
  std::string type;
  //  fs::path path;
};

class Mesh {
public:
  // mesh Data
  //  std::vector<details::Vertex> vertices;
  //  std::vector<unsigned int> indices;
  std::vector<details::Texture> textures_;
  int indices_nums_ = 0;
  // constructor
  Mesh(const std::vector<Vertex> &vertices,
       const std::vector<unsigned int> &indices, std::vector<Texture> textures);

  // render the mesh
  void Draw(const Program &program);
  VertexArray vao_;
};
} // namespace details

class Model {
public:
  // model data
  std::unordered_map<std::string, details::Texture>
      textures_loaded; // stores all the textures loaded so far, optimization to
                       // make sure textures aren't loaded more than once.
  std::vector<details::Mesh> meshes;
  fs::path directory{};
  bool gammaCorrection_ = false;

  // constructor, expects a filepath to a 3D model.
  Model(std::string const &path, bool gamma = false) : gammaCorrection_(gamma) {
    loadModel(path);
  }

  // draws the model, and thus all its meshes
  void Draw(const Program &prog) {
    for (auto &mesh : meshes)
      mesh.Draw(prog);
  }

private:
  // loads a model with supported ASSIMP extensions from file and stores the
  // resulting meshes in the meshes vector.
  void loadModel(const fs::path &path);

  // processes a node in a recursive fashion. Processes each individual mesh
  // located at the node and repeats this process on its children nodes (if
  // any).
  void processNode(aiNode *node, const aiScene *scene);

  details::Mesh processMesh(aiMesh *mesh, const aiScene *scene);

  // checks all material textures of a given type and loads the textures if
  // they're not loaded yet. the required info is returned as a Texture struct.
  std::vector<details::Texture>
  loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                       const std::string &typeName);
};

inline float lerp(float a, float b, float t) { return a * (1 - t) + b * t; }

#ifndef NDEBUG
void PostCallbackFunc(const char *name, void *funcptr, int len_args, ...);
#endif
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam);
// clang-format on


class RenderBase {
private:
  void InitWindow();

public:
  struct BaseInfo {
    std::string title = "DiRenderLab";
    std::string glsl_version = "#version 450";
    int width = 1600;
    int height = 900;
    int major_version = 4;
    int minor_version = 5;
    bool resizeable = true;
#ifdef NDEBUG
    bool debug = false;
#else
    bool debug = true;
#endif
  };

protected:
  BaseInfo info_;
  bool AllowMouseMove_ = true;
  float lastX_ = (float)info_.width / 2.0;
  float lastY_ = (float)info_.height / 2.0;
  bool firstMouse_ = true;
  float deltaTime_ = 0.0f;
  float lastFrame_ = 0.0f;
  GLFWwindow *window_ = nullptr;

  virtual void on_resize(int width, int height);
  virtual void on_key(int key, int scancode, int action, int mods);
  virtual void on_mouse_scroll(double xoffset, double yoffset);
  virtual void on_mouse_move(double xpos, double ypos);
  virtual void processInput();
  virtual void shutdown() { /*some user-defined code here*/
    if (DRL::Program::current_using_program) {
      DRL::Program::current_using_program->unbind();
    }
  }
  virtual void render() = 0;
  virtual void setup_states() {}

public:
  std::unique_ptr<Camera> camera_ = nullptr;
  // timing
  RenderBase();
  RenderBase(BaseInfo info);
  virtual ~RenderBase();
  virtual void loop() final;

  //        void framebuffer_size_callback(GLFWwindow *window, int width, int
  //        height); void mouse_callback(GLFWwindow *window, double xpos, double
  //        ypos); void scroll_callback(GLFWwindow *window, double xoffset,
  //        double yoffset); void key_callback(GLFWwindow *window, int key, int
  //        scancode, int action,
  //                          int mods);
  // settings

  // camera
};
} // namespace DRL
