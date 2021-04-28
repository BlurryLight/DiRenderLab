#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLwrapper/glsupport.hh"
#include "GLwrapper/program.hh"
#include "GLwrapper/shapes.hh"
#include "GLwrapper/texture.hh"
#include "GLwrapper/vertex_array.h"
#include "GLwrapper/vertex_buffer.hh"
#include "utils/resource_path_searcher.h"
using DRL::Camera;
using DRL::Shader;

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);
static bool AllowMouseMove = true;
void processInput(GLFWwindow *window);
void renderScene(const DRL::Program &shader);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
DRL::Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float) SCR_WIDTH / 2.0;
float lastY = (float) SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// meshes
DRL::VertexArray *planeVAO;

int main() {
    //spdlog init
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("log.txt", true));
    auto combined_logger = std::make_shared<spdlog::logger>("basic_logger", begin(sinks), end(sinks));
    //register it if you need to access it globally
    spdlog::register_logger(combined_logger);
    spdlog::set_default_logger(combined_logger);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glad_set_post_callback(DRL::PostCallbackFunc);

#ifndef NDEBUG
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);// makes sure errors are displayed synchronously
        glDebugMessageCallback(DRL::glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif
    const char *glsl_verson = "#version 330";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_verson);

    // IMGUI
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    DRL::ResourcePathSearcher resMgr;
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" / "shadowmap");
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
    // build and compile shaders
    // -------------------------
    DRL::Program shader = DRL::make_program(
            resMgr.find_path("shadow_mapping.vert"),
            resMgr.find_path("shadow_mapping.frag"));
    DRL::Program simpleDepthShader = DRL::make_program(
            resMgr.find_path("shadow_mapping_depth.vert"),
            resMgr.find_path("shadow_mapping_depth.frag"));

    DRL::Program DepthMapShader = DRL::make_program(
            resMgr.find_path("debug_quad.vert"),
            resMgr.find_path("debug_quad_depth.frag"));

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,

            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
            25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f};
    // plane VAO

    DRL::VertexBuffer planeVBO(planeVertices, sizeof(planeVertices), DRL::kStaticDraw);
    //    DRL::VertexArray planeVAO;
    planeVAO = new DRL::VertexArray();
    planeVAO->lazy_bind_attrib(0, GL_FLOAT, 3, 0);
    planeVAO->lazy_bind_attrib(1, GL_FLOAT, 3, 3);
    planeVAO->lazy_bind_attrib(2, GL_FLOAT, 2, 6);
    planeVAO->update_bind(planeVBO, 0, 8);


    // load textures
    // -------------
    DRL::Texture2D woodTexture = DRL::Texture2D(resMgr.find_path("wood.png"), false, true);

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture

    DRL::Texture2D depthMap(SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT24, GL_FLOAT, nullptr);
    depthMap.set_wrap_s(GL_CLAMP_TO_EDGE);
    depthMap.set_wrap_t(GL_CLAMP_TO_EDGE);
    depthMap.bind();

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // shader configuration
    // --------------------
    shader.use();
    shader.set_uniform("diffuseTexture", 0);
    shader.set_uniform("shadowMap", 1);


    // lighting info
    // -------------
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

    float uBias = 0.05;
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Background Color", 0);// Create a window called "Hello,
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::SliderFloat("ShadowMap bias", &uBias, 0.0f, 1.0f, "bias = %.3f");
            ImGui::End();
        }
        ImGui::Render();


        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        // render scene from light's point of view
        simpleDepthShader.use();
        simpleDepthShader.set_uniform("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        woodTexture.bind();
        renderScene(simpleDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. render scene as normal using the generated depth/shadow map
        // --------------------------------------------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.set_uniform("projection", projection);
        shader.set_uniform("view", view);
        // set light uniforms
        shader.set_uniform("viewPos", camera.Position);
        shader.set_uniform("lightPos", lightPos);
        shader.set_uniform("lightSpaceMatrix", lightSpaceMatrix);
        shader.set_uniform("uBias", uBias);
        woodTexture.set_slot(0);
        woodTexture.bind();
        depthMap.set_slot(1);
        depthMap.bind();
        //        glActiveTexture(GL_TEXTURE0);
        //        glBindTexture(GL_TEXTURE_2D, woodTexture);
        //        glActiveTexture(GL_TEXTURE1);
        //        glBindTexture(GL_TEXTURE_2D, depthMap);
        renderScene(shader);
        depthMap.unbind();

        // render Depth map to quad for visual debugging
        // ---------------------------------------------
        //        DepthMapShader.use();
        //        DepthMapShader.set_uniform("near_plane", near_plane);
        //        DepthMapShader.set_uniform("far_plane", far_plane);
        //        depthMap.set_slot(0);
        //        depthMap.bind();
        //        glActiveTexture(GL_TEXTURE0);
        //        glBindTexture(GL_TEXTURE_2D, depthMap);
        //        DRL::renderQuad();
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    //    glDeleteVertexArrays(1, &planeVAO);
    //    glDeleteBuffers(1, &planeVBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const DRL::Program &shader) {
    // floor
    glm::mat4 model = glm::mat4(1.0f);
    shader.set_uniform("model", model);
    {
        DRL::bind_guard gd(*planeVAO);
        planeVAO->draw(GL_TRIANGLES, 0, 6);
    }
    //    glBindVertexArray(*planeVAO);
    //    glDrawArrays(GL_TRIANGLES, 0, 6);
    // cubes
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.set_uniform("model", model);
    DRL::renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.set_uniform("model", model);
    DRL::renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.25));
    shader.set_uniform("model", model);
    //    renderCube();
    DRL::renderSphere();
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(DRL::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(DRL::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(DRL::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(DRL::RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (AllowMouseMove) {

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;// reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    } else {
        firstMouse = true;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        AllowMouseMove = !AllowMouseMove;
        if (AllowMouseMove)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
