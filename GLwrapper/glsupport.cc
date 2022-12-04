//
// Created by zhong on 2021/4/26.
//

#include <utility>
#include <utils/cmake_vars.h>
#include <glm/gtx/quaternion.hpp>

#include "glsupport.hh"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
using namespace DRL;

using DRL::details::Mesh;
using DRL::details::Vertex;

float DRL::get_random_float(float min, float max) {
  static std::mt19937 generator;
  std::uniform_real_distribution<float> dis(min, max);
  return dis(generator);
}
void DRL::glDebugOutput(GLenum source, GLenum type, unsigned int id,
                        GLenum severity, GLsizei length, const char *message,
                        const void *userParam) {
  // clang-format off
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::stringstream ss;
    ss << "---------------" << "\n";
    ss << "Debug message (" << id << "): " <<  message << "\n";
    // clang-format off
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             ss << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ss << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     ss << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     ss << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           ss << "Source: Other"; break;
    } ss << "\n";

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               ss << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ss << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         ss << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         ss << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              ss << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          ss << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           ss << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               ss << "Type: Other"; break;
    } ss << "\n";

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         ss << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       ss << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          ss << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: ss << "Severity: notification"; break;
    } ss << "\n" <<std::endl;
  // clang-format on
  spdlog::error(ss.str());
  if (severity == GL_DEBUG_SEVERITY_HIGH ||
      severity == GL_DEBUG_SEVERITY_MEDIUM) {
    spdlog::shutdown();
    std::quick_exit(-1);
  }
}

#ifndef NDEBUG
void DRL::PostCallbackFunc(const char *name, void *funcptr, int len_args, ...) {
  (void)funcptr;
  (void)len_args;
  GLenum errorCode;
  if ((errorCode = glad_glGetError()) != GL_NO_ERROR) {
    std::string error;
    switch (errorCode) {
    case GL_INVALID_ENUM:
      error = "INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      error = "INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      error = "INVALID_OPERATION";
      break;
    case GL_STACK_OVERFLOW:
      error = "STACK_OVERFLOW";
      break;
    case GL_STACK_UNDERFLOW:
      error = "STACK_UNDERFLOW";
      break;
    case GL_OUT_OF_MEMORY:
      error = "OUT_OF_MEMORY";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      error = "INVALID_FRAMEBUFFER_OPERATION";
      break;
    }
    spdlog::error("Error happens: {} {}", name, error);
  }
}
#endif

void Model::loadModel(const fs::path &path) {
  // read file via ASSIMP
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path.string(),
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
  // check for errors
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) // if is Not Zero
  {
    std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
    return;
  }
  // retrieve the directory path of the filepath
  directory = fs::absolute(path.parent_path());

  // process ASSIMP's root node recursively
  processNode(scene->mRootNode, scene);
}
void Model::processNode(aiNode *node, const aiScene *scene) {
  // process each mesh located at the current node
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    // the node object only contains indices to index the actual objects in the
    // scene. the scene contains all the data, node is just to keep stuff
    // organized (like relations between nodes).
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh, scene));
  }
  // after we've processed all of the meshes (if any) we then recursively
  // process each of the children nodes
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene);
  }
}
Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
  // data to fill
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<details::Texture> textures;

  // walk through each of the mesh's vertices
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex{};
    glm::vec3 vector; // we declare a placeholder vector since assimp uses its
                      // own vector class that doesn't directly convert to glm's
                      // vec3 class so we transfer the data to this placeholder
                      // glm::vec3 first.
    // positions
    vector.x = mesh->mVertices[i].x;
    vector.y = mesh->mVertices[i].y;
    vector.z = mesh->mVertices[i].z;
    vertex.Position = vector;
    // normals
    vector.x = mesh->mNormals[i].x;
    vector.y = mesh->mNormals[i].y;
    vector.z = mesh->mNormals[i].z;
    vertex.Normal = vector;
    // texture coordinates
    if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
    {
      glm::vec2 vec;
      // a vertex can contain up to 8 different texture coordinates. We thus
      // make the assumption that we won't use models where a vertex can have
      // multiple texture coordinates so we always take the first set (0).
      vec.x = mesh->mTextureCoords[0][i].x;
      vec.y = mesh->mTextureCoords[0][i].y;
      vertex.TexCoords = vec;
    } else
      vertex.TexCoords = glm::vec2(0.0f, 0.0f);
    // tangent
    vector.x = mesh->mTangents[i].x;
    vector.y = mesh->mTangents[i].y;
    vector.z = mesh->mTangents[i].z;
    vertex.Tangent = vector;
    // bitangent
    vector.x = mesh->mBitangents[i].x;
    vector.y = mesh->mBitangents[i].y;
    vector.z = mesh->mBitangents[i].z;
    vertex.Bitangent = vector;
    vertices.push_back(vertex);
  }
  // now wak through each of the mesh's faces (a face is a mesh its triangle)
  // and retrieve the corresponding vertex indices.
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    // retrieve all indices of the face and store them in the indices vector
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      indices.push_back(face.mIndices[j]);
  }
  // process materials
  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  // we assume a convention for sampler names in the shaders. Each diffuse
  // texture should be named as 'texture_diffuseN' where N is a sequential
  // number ranging from 1 to MAX_SAMPLER_NUMBER. Same applies to other texture
  // as the following list summarizes: diffuse: texture_diffuseN specular:
  // texture_specularN normal: texture_normalN

  // 1. diffuse maps
  auto diffuseMaps =
      loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
  textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
  // 2. specular maps
  auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR,
                                           "texture_specular");
  textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
  // 3. normal maps
  auto normalMaps =
      loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
  textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
  // 4. height maps
  auto heightMaps =
      loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
  textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

  // return a mesh object created from the extracted mesh data
  return Mesh(vertices, indices, textures);
}
std::vector<details::Texture>
Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                            const std::string &typeName) {
  std::vector<details::Texture> res;
  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString texture_path;
    mat->GetTexture(type, i, &texture_path);

    // check if texture was loaded before and if so, continue to next iteration:
    // skip loading a new texture
    bool skip = false;
    std::string texture_path_str(texture_path.C_Str());
    if (textures_loaded.find(texture_path_str) != textures_loaded.end()) {
      res.push_back(textures_loaded[texture_path_str]);
      skip = true; // a texture with the same filepath has already been
    }
    if (!skip) { // if texture hasn't been loaded already, load it
      details::Texture texture;
      auto path = directory / texture_path_str;
      texture.tex_ptr =
          std::make_shared<DRL::Texture2D>(path, 3, gammaCorrection_, true);
      texture.tex_ptr->generateMipmap();
      texture.type = typeName;
      //      texture.path = str.C_Str();
      res.push_back(texture);
      textures_loaded.emplace(path.string(), texture);
      // store it as texture loaded for entire model, to ensure we
    }
  }
  return res;
}
Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<unsigned int> &indices,
           std::vector<Texture> textures)
    : textures_(std::move(textures)),
      indices_nums_(static_cast<int>(indices.size())) {
  auto vbo = std::make_shared<DRL::VertexBuffer>(
      vertices.data(), vertices.size() * sizeof(details::Vertex),
      GL_DYNAMIC_STORAGE_BIT);
  auto ebo = std::make_shared<DRL::ElementBuffer>(
      indices.data(), indices.size() * sizeof(unsigned int),
      GL_DYNAMIC_STORAGE_BIT);

  vao_.lazy_bind_attrib(0, GL_FLOAT, 3, 0);  // vertex
  vao_.lazy_bind_attrib(1, GL_FLOAT, 3, 3);  // normal
  vao_.lazy_bind_attrib(2, GL_FLOAT, 2, 6);  // texcoords
  vao_.lazy_bind_attrib(3, GL_FLOAT, 3, 8);  // tangent
  vao_.lazy_bind_attrib(4, GL_FLOAT, 3, 11); // bitangent
  int num_of_elems = sizeof(Vertex) / sizeof(float);
  int elem_bytes = sizeof(float);
  vao_.update_bind(vbo, ebo, 0, num_of_elems, elem_bytes);
}
void Mesh::Draw(const Program &program) {
  // bind appropriate textures
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;
  for (unsigned int i = 0; i < textures_.size(); i++) {
    std::string number;
    std::string name = textures_[i].type;
    if (name == "texture_diffuse")
      number = std::to_string(diffuseNr++);
    else if (name == "texture_specular")
      number = std::to_string(specularNr++); // transfer unsigned int to stream
    else if (name == "texture_normal")
      number = std::to_string(normalNr++); // transfer unsigned int to stream
    else if (name == "texture_height")
      number = std::to_string(heightNr++); // transfer unsigned int to stream

    // now set the sampler to the correct texture unit
    //    glUniform1i(glGetUniformLocation(program.handle(), (name +
    //    number).c_str()),
    //                i);
    program.set_uniform(name + number, i);
    textures_[i].tex_ptr->set_slot(i);
    textures_[i].tex_ptr->bind();
  }
  {
    DRL::bind_guard gd(vao_);
    vao_.draw(GL_TRIANGLES, indices_nums_, GL_UNSIGNED_INT, nullptr);
  }
  for (auto &texture : textures_) {
    texture.tex_ptr->unbind();
  }
}
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = MovementSpeed * deltaTime;
  if (direction == FORWARD)
    Position += Front * velocity;
  if (direction == BACKWARD)
    Position -= Front * velocity;
  if (direction == LEFT)
    Position -= Right * velocity;
  if (direction == RIGHT)
    Position += Right * velocity;
}
void Camera::ProcessMouseMovement(float xoffset, float yoffset,
                                  GLboolean constrainPitch) {
  xoffset *= MouseSensitivity;
  yoffset *= MouseSensitivity;

  Yaw += xoffset;
  Pitch += yoffset;

  // Make sure that when pitch is out of bounds, screen doesn't get flipped
  if (constrainPitch) {
    if (Pitch > 89.0f)
      Pitch = 89.0f;
    if (Pitch < -89.0f)
      Pitch = -89.0f;
  }

  // Update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}
void Camera::ProcessMouseScroll(float yoffset) {
  if (Zoom >= 1.0f && Zoom <= 45.0f)
    Zoom -= yoffset;
  if (Zoom <= 1.0f)
    Zoom = 1.0f;
  if (Zoom >= 45.0f)
    Zoom = 45.0f;
}
void Camera::updateCameraVectors() {
  // Calculate the new Front vector
  glm::vec3 front;
  front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  front.y = sin(glm::radians(Pitch));
  front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  Front = glm::normalize(front);
  // Also re-calculate the Right and Up vector
  Right = glm::normalize(glm::cross(
      Front, WorldUp)); // Normalize the vectors, because their length gets
                        // closer to 0 the more you look up or down which
                        // results in slower movement.
  Up = glm::normalize(glm::cross(Right, Front));

// Front = glm::vec3(0.0f, 0.0f, -1.0f);
//   Up = glm::normalize(WorldUp);
//   Right = glm::normalize(glm::cross(Front,WorldUp));
//   glm::quat pitch_quat = glm::angleAxis(glm::radians(Pitch), Right);
//   glm::quat yaw_quat = glm::angleAxis(glm::radians(Yaw), Up);
//   glm::quat temp = glm::cross(pitch_quat,yaw_quat);
//   temp = glm::normalize(temp);
//   Front = glm::rotate(temp,Front);
}
Camera::Camera(float posX, float posY, float posZ, float upX, float upY,
               float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  Position = glm::vec3(posX, posY, posZ);
  WorldUp = glm::vec3(upX, upY, upZ);
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  Position = position;
  WorldUp = up;
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}
RenderBase::RenderBase() : RenderBase(BaseInfo{}) {}
RenderBase::RenderBase(BaseInfo info) : info_(std::move(info)) {
  InitWindow();
  spdlog::info("Root Dir: {}", DRL::ROOT_DIR);
  spdlog::info("Compiler: {} {}, Build Type: {}", BUILD_COMPILER, CXX_VER,
               BUILD_TYPE);
  spdlog::info("System: {} {}, Build UTC Time: {}", BUILD_SYSTEM_NAME,
               BUILD_SYSTEM_VERSION, BUILD_UTC_TIMESTAMP);
}
void RenderBase::InitWindow() {
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, info_.major_version);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, info_.minor_version);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, info_.debug);

  // glfw window creation
  // --------------------
  GLFWwindow *window = glfwCreateWindow(info_.width, info_.height,
                                        info_.title.c_str(), nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    std::quick_exit(-1);
  }
  window_ = window;
  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);
  auto resize_cb = [](GLFWwindow *win, int width, int height) {
    static_cast<RenderBase *>(glfwGetWindowUserPointer(win))
        ->on_resize(width, height);
  };

  auto mouse_cb = [](GLFWwindow *win, double xpos, double ypos) {
    static_cast<RenderBase *>(glfwGetWindowUserPointer(win))
        ->on_mouse_move(xpos, ypos);
  };

  auto scroll_cb = [](GLFWwindow *win, double xoffset, double yoffset) {
    static_cast<RenderBase *>(glfwGetWindowUserPointer(win))
        ->on_mouse_scroll(xoffset, yoffset);
  };

  auto key_cb = [](GLFWwindow *win, int key, int scancode, int action,
                   int mods) {
    static_cast<RenderBase *>(glfwGetWindowUserPointer(win))
        ->on_key(key, scancode, action, mods);
  };

  glfwSetFramebufferSizeCallback(window, resize_cb);
  glfwSetCursorPosCallback(window, mouse_cb);
  glfwSetKeyCallback(window, key_cb);
  glfwSetScrollCallback(window, scroll_cb);

  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    std::quick_exit(-1);
  }
#ifndef NDEBUG
  if (info_.debug)
    glad_set_post_callback(DRL::PostCallbackFunc);
#endif

  if (info_.debug) {
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are
                                             // displayed synchronously
      glDebugMessageCallback(DRL::glDebugOutput, nullptr);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                            nullptr, GL_TRUE);
    }
  }
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(info_.glsl_version.c_str());

  spdlog::info("Vendor: {}", (char*)(glGetString(GL_VENDOR)));
  spdlog::info("Renderer: {}", (char*)(glGetString(GL_RENDERER)));
  spdlog::info("Version: {}", (char*)(glGetString(GL_VERSION)));
  spdlog::info("GLSL : {}", (char*)(glGetString(GL_SHADING_LANGUAGE_VERSION)));
}
RenderBase::~RenderBase() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}
void RenderBase::loop() {
  AssertLog(bool(camera_), "Camera is uninitialized when loop");
  setup_states();
  while (!glfwWindowShouldClose(window_)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    auto currentFrame = (float)glfwGetTime();
    deltaTime_ = currentFrame - lastFrame_;
    lastFrame_ = currentFrame;
    render();
    processInput();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
    glfwPollEvents();
  }
  shutdown();
}
void RenderBase::on_resize(int width, int height) {
  glViewport(0, 0, width, height);
  info_.width = width;
  info_.height = height;
  lastX_ = (float)info_.width / 2.0f;
  lastY_ = (float)info_.height / 2.0f;
}
void RenderBase::on_key(int key, int scancode, int action, int mods) {
  (void)mods;
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    AllowMouseMove_ = !AllowMouseMove_;
    if (AllowMouseMove_)
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}
void RenderBase::on_mouse_scroll(double xoffset, double yoffset) {
  (void)xoffset;
  if (camera_)
    camera_->ProcessMouseScroll(yoffset);
}
void RenderBase::on_mouse_move(double xpos, double ypos) {
  if (!camera_)
    return;
  if (AllowMouseMove_) {
    if (firstMouse_) {
      lastX_ = xpos;
      lastY_ = ypos;
      firstMouse_ = false;
    }

    float xoffset = xpos - lastX_;
    float yoffset =
        lastY_ - ypos; // reversed since y-coordinates go from bottom to top

    lastX_ = xpos;
    lastY_ = ypos;

    camera_->ProcessMouseMovement(xoffset, yoffset);
  } else {
    firstMouse_ = true;
  }
}
void RenderBase::processInput() {
  if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window_, true);

  if (!camera_)
    return;
  // J for speed up
  // K for speed down
  if (glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS) {
    camera_->MovementSpeed += 1.0;
    spdlog::info("Current camera speed:{}", camera_->MovementSpeed);
  }
  if (glfwGetKey(window_, GLFW_KEY_K) == GLFW_PRESS) {
    camera_->MovementSpeed = std::min(camera_->MovementSpeed - 1.0f, 0.0f);
    spdlog::info("Current camera speed:{}", camera_->MovementSpeed);
  }
  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
    camera_->ProcessKeyboard(DRL::FORWARD, deltaTime_);
  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
    camera_->ProcessKeyboard(DRL::BACKWARD, deltaTime_);
  if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
    camera_->ProcessKeyboard(DRL::LEFT, deltaTime_);
  if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
    camera_->ProcessKeyboard(DRL::RIGHT, deltaTime_);
}
