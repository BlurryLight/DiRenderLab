#pragma once

#include "program.hh"
#include "shader.hh"
#include "vertex_array.h"
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glad/glad.h>
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
#include <vector>
namespace DRL {
    // Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    // Default camera values
    constexpr inline float YAW = -90.0f;
    constexpr inline float PITCH = 0.0f;
    constexpr inline float SPEED = 2.5f;
    constexpr inline float SENSITIVITY = 0.1f;
    constexpr inline float ZOOM = 45.0f;

    // classes
    class Camera;
    class Mesh;
    class Shader;
    struct Vertex;
    struct Texture1;
    class Model;

    float get_random_float(float min = 0.0f, float max = 1.0f);


    // An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
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
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
        // Constructor with scalar values
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

        // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 GetViewMatrix() {
            return glm::lookAt(Position, Position + Front, Up);
        }

        // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        void ProcessKeyboard(Camera_Movement direction, float deltaTime);

        // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

        // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void ProcessMouseScroll(float yoffset);

    private:
        // Calculates the front vector from the Camera's (updated) Euler Angles
        void updateCameraVectors();
    };


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

    struct Texture1 {
        unsigned int id;
        std::string type;
        std::string path;
    };

    class Mesh {
    public:
        // mesh Data
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture1> textures;
        unsigned int VAO{};

        // constructor
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture1> textures);

        // render the mesh
        void Draw(const Program &program);


    private:
        // render data
        unsigned int VBO{}, EBO{};

        // initializes all the buffer objects/arrays
        void setupMesh();
    };
    class Model {
    public:
        // model data
        std::vector<Texture1> textures_loaded;// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
        std::vector<Mesh> meshes;
        std::string directory;
        bool gammaCorrection;

        // constructor, expects a filepath to a 3D model.
        Model(std::string const &path, bool gamma = false) : gammaCorrection(gamma) {
            loadModel(path);
        }

        // draws the model, and thus all its meshes
        void Draw(const Program &prog) {
            for (unsigned int i = 0; i < meshes.size(); i++)
                meshes[i].Draw(prog);
        }

    private:
        // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
        void loadModel(std::string const &path);

        // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
        void processNode(aiNode *node, const aiScene *scene);

        Mesh processMesh(aiMesh *mesh, const aiScene *scene);

        // checks all material textures of a given type and loads the textures if they're not loaded yet.
        // the required info is returned as a Texture struct.
        std::vector<Texture1> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    };


    unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma, bool flip);

    inline float lerp(float a, float b, float t) {
        return a * (1 - t) + b * t;
    }

    void PostCallbackFunc(const char *name, void *funcptr, int len_args, ...);
    void APIENTRY glDebugOutput(GLenum source,
                                GLenum type,
                                unsigned int id,
                                GLenum severity,
                                GLsizei length,
                                const char *message,
                                const void *userParam);
    // clang-format on

}//namespace DRL
