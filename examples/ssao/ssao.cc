#include <GLwrapper/glsupport.hh>
#include <GLwrapper/shapes.hh>
#include <filesystem>
#include <iostream>
using namespace DRL;
using uint = unsigned int;
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

enum { kNormal = 0,
       kPosition,
       kSSAO,
       kSSAOBLURRED };
auto mode = kSSAO;

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SSAO", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------

    DRL::ResourcePathSearcher resMgr;
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" / "ssao");
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "models" / "spot");
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");

    auto create_prog = [](const fs::path &vert, const fs::path &frag) {
        DRL::Program prog;
        Shader vshader(GL_VERTEX_SHADER, vert);
        Shader fshader(GL_FRAGMENT_SHADER, frag);
        prog.attach_shaders({vshader, fshader});
        prog.link();
        return prog;
    };

    auto gShader = create_prog(resMgr.find_path("gbuffer.vert"), resMgr.find_path("gbuffer.frag"));
    auto ssaoShader = create_prog(resMgr.find_path("ssao.vert"), resMgr.find_path("ssao.frag"));
    auto blurShader = create_prog(resMgr.find_path("ssao.vert"), resMgr.find_path("blur.frag"));
    auto visFloatShader = create_prog(resMgr.find_path("ssao.vert"), resMgr.find_path("simple_float_test.frag"));
    auto visVecShader = create_prog(resMgr.find_path("ssao.vert"), resMgr.find_path("simple_vec3_test.frag"));

    // load models
    // -----------
    Model spotModel(resMgr.find_path("spot_triangulated_good.obj").string());
    auto texture_path = resMgr.find_path("spot_texture.png").string();
    auto spot_texture = TextureFromFile(texture_path.c_str(), "", false, false);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spot_texture);

    // configure gBuffer
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    uint gPosition, gNormal, gAlbedo;
    auto init_texture = [](uint &buffer, GLint internalFormat, GLint pixelType) {
        glGenTextures(1, &buffer);
        glBindTexture(GL_TEXTURE_2D, buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, pixelType, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };
    init_texture(gPosition, GL_RGB16F, GL_FLOAT);
    init_texture(gNormal, GL_RGB16F, GL_FLOAT);
    init_texture(gAlbedo, GL_RGB, GL_UNSIGNED_BYTE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    uint attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);
    //attach depthbuffer
    uint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("FRAMEBUFFER NOT COMPLETE!");
        std::abort();
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //config SSAO buffer
    uint ssaoFBO;
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    uint ssaoColorBuffer;
    init_texture(ssaoColorBuffer, GL_RED, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("FRAMEBUFFER NOT COMPLETE!");
        std::abort();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //config noise
    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < 16; i++) {
        glm::vec3 noise(get_random_float(-1.0, 1.0), get_random_float(-1.0, 1.0), 0.0);
        ssaoNoise.push_back(noise);
    }
    uint noiseTexture;
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    // hemisphere samples
    std::vector<glm::vec3> samples;
    uint sample_nums = 64;
    for (int i = 0; i < sample_nums; i++) {
        //在2x2x1的立方体内采样
        glm::vec3 sample(get_random_float(-1.0f, 1.0f), get_random_float(-1.0f, 1.0f), get_random_float());
        //normalize到r=1的球面上
        sample = glm::normalize(sample);
        //缩放到球面内
        sample *= get_random_float();
        //把sample更加趋近于表面//
        sample *= (float(i) / sample_nums) * (float(i) / sample_nums);
        samples.push_back(sample);
    }
    //config blur buffer(径向模糊)
    uint blurFBuffer;
    glGenFramebuffers(1, &blurFBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBuffer);
    uint ssaoColorBufferBlurred;
    init_texture(ssaoColorBufferBlurred, GL_RED, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlurred, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("FRAMEBUFFER NOT COMPLETE!");
        std::abort();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //shader config
    //gbuffer test
    ssaoShader.use();
    ssaoShader.set_uniform("gPosition", 0);
    ssaoShader.set_uniform("gNormal", 1);
    ssaoShader.set_uniform("tNoise", 2);
    //blur
    blurShader.use();
    blurShader.set_uniform("ssaoTexInput", 0);
    //
    visFloatShader.use();
    visFloatShader.set_uniform("gbuffer_test", 0);
    visVecShader.use();
    visVecShader.set_uniform("gbuffer_test", 0);
    // visShader.setInt("ssao",0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
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
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        gShader.set_uniform("projection", projection);
        gShader.set_uniform("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));// translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));    // it's a bit too big for our scene, so scale it down
        gShader.set_uniform("invertNormal", 1);
        gShader.set_uniform("model", model);
        renderCube();

        gShader.set_uniform("invertNormal", 0);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.2f, 0.0f));// translate it down so it's at the center of the scene
        gShader.set_uniform("model", model);
        renderSphere();


        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(1.0));// it's a bit too big for our scene, so scale it down
        gShader.set_uniform("model", model);

        spotModel.Draw(gShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //ssao
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoShader.use();
        // view/projection transformations
        ssaoShader.set_uniform("projection", projection);

        // render the loaded model
        for (int i = 0; i < sample_nums; i++) {
            ssaoShader.set_uniform("samples[" + std::to_string(i) + "]", samples[i]);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBuffer);
        glClear(GL_COLOR_BUFFER_BIT);
        blurShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //blur stage
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (mode == kSSAOBLURRED || mode == kSSAO) {
            visFloatShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, (mode == kSSAO ? ssaoColorBuffer : ssaoColorBufferBlurred));
            std::cout << "Current Mode" << ((mode == kSSAO) ? "SSAO" : "SSAO_BLURRED") << std::endl;
        } else if (mode == kNormal || mode == kPosition) {
            visVecShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, (mode == kNormal ? gNormal : gPosition));
            std::cout << "Current Mode" << ((mode == kNormal) ? "kNormal" : "Position") << std::endl;
        }
        renderQuad();


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        int mode_num = static_cast<int>(mode);
        mode_num++;
        if (mode_num > 3) mode_num = 0;
        mode = static_cast<decltype(kNormal)>(mode_num);
    }
}
