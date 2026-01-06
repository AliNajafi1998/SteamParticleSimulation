#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
// Glad must be included before GLFW
#include "room/Room.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// Window dimensions
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera
#include "camera/Camera.h"
#include "engine/DensityVolume.h" // [NEW] Volumetric
#include "engine/SteamEngine.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "room/Kurna.h"
#include "room/Room.h"
#include <glad/glad.h>
#include <iostream>
// Initial position: (0.0f, -10.0f, 40.0f) - Close to floor, outside looking in
// Up vector: (0.0f, 1.0f, 0.0f)
// Yaw: -90.0f, Pitch: 0.0f
Camera camera(0.0f, -10.0f, 40.0f, 0.0f, 1.0f, 0.0f, -90.0f, 0.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cameraEnabled = true; // [NEW] Toggle for ImGui mouse usage
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int createShader(const char *vertexPath, const char *fragmentPath);

int main() {
  // Initialize GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Create window
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Turkish Bath", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  // Capture mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Load GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Configure global opengl state
  glEnable(GL_DEPTH_TEST);
  // glIgnore(GL_CULL_FACE); // Disabled to ensure walls are visible from both
  // sides

  // Shader
  unsigned int shaderProgram =
      createShader("src/shaders/simple.vert", "src/shaders/simple.frag");

  // Initialize Room (Width, Height, Depth)
  Room room(50.0f, 30.0f, 50.0f); // [RESIZED] Width/Depth 50, Height 30
  room.setTemperature(25.0f);

  // Initialize Kurna (Radius, Height)
  Kurna kurna(2.0f, 1.0f);

  // [NEW] Volumetric Texture setup
  DensityVolume densityVolume(64, 64, 64);
  unsigned int volTexture;
  glGenTextures(1, &volTexture);
  glBindTexture(GL_TEXTURE_3D, volTexture);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // [NEW] Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  bool debugMode =
      true; // Toggle between Particles (True) and Volumetric (False)

  // Main loop
  // Initial empty upload to define format
  int dw, dh, dd;
  densityVolume.getParams(&dw, &dh, &dd);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, dw, dh, dd, 0, GL_RED, GL_FLOAT,
               NULL);
  glBindTexture(GL_TEXTURE_3D, 0);

  // Volumetric Shader
  unsigned int volShader = createShader("src/shaders/volumetric.vert",
                                        "src/shaders/volumetric.frag");

  // Volume Cube VAO (Simple cube -1 to 1)
  float cubeVertices[] = {
      // positions
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
  unsigned int cubeVAO, cubeVBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);

  // Initialize Steam Engine
  SteamEngine steamEngine;
  steamEngine.Initialize(2000000); // Start with capacity for 2000 particles

  // Render Loop
  while (!glfwWindowShouldClose(window)) {
    // Per-frame time logic
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // UI Window
    ImGui::Begin("Controller");
    ImGui::Text("Sim Stats");

    int uiActiveCount = 0;
    for (const auto &p : steamEngine.getParticles())
      if (p.active)
        uiActiveCount++;
    ImGui::Text("Active Particles: %d", uiActiveCount);

    ImGui::Separator();
    ImGui::Checkbox("Debug Mode (Particles)", &debugMode);

    ImGui::Separator();
    ImGui::Text("Physics Parameters");
    ImGui::SliderFloat("Gravity", &steamEngine.gravity, -10.0f, 1.0f);
    ImGui::SliderFloat("Buoyancy", &steamEngine.buoyancyCoeff, 0.0f, 10.0f);
    ImGui::SliderFloat("Cooling Rate", &steamEngine.coolingRate, 0.0f, 2.0f);
    ImGui::SliderFloat("Emission Rate", &steamEngine.emissionRate, 10.0f,
                       1000.0f);

    // [NEW] Ray Marching Step Size
    static float rayStepSize = 0.5f;
    ImGui::SliderFloat("Ray Step Size", &rayStepSize, 0.05f, 2.0f);

    ImGui::End();

    // Make sure to propagate ImGui input capture to camera?
    // If ImGui wants mouse, don't let camera take it.
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      processInput(window);
    }

    // Update Steam Simulation
    steamEngine.Update(deltaTime);

    // [NEW] Update Density Volume
    densityVolume.Build(steamEngine.getParticles());
    const auto &volData = densityVolume.getData();
    glBindTexture(GL_TEXTURE_3D, volTexture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, dw, dh, dd, GL_RED, GL_FLOAT,
                    volData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // DEBUG: Print status every 1 second
    static float debugTimer = 0.0f;
    debugTimer += deltaTime;
    if (debugTimer > 1.0f) {
      debugTimer = 0.0f;
      const auto &particles = steamEngine.getParticles();
      int activeCount = 0;
      for (const auto &p : particles) {
        if (p.active)
          activeCount++;
      }
      std::cout << "[DEBUG] Active Particles: " << activeCount << std::endl;

      // Print first active particle pos
      for (const auto &p : particles) {
        if (p.active) {
          std::cout << "   Sample Pos: (" << p.position[0] << ", "
                    << p.position[1] << ", " << p.position[2] << ")"
                    << " Temp: " << p.temperature << " Life: " << p.life
                    << std::endl;
          break;
        }
      }
    }

    // Render
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Transformation matrices
    mat4 projection;
    glm_perspective(glm_rad(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT,
                    0.1f, 100.0f, projection);

    mat4 view;
    camera.GetViewMatrix(view);

    mat4 model;
    glm_mat4_identity(model);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float *)model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (float *)view);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, (float *)projection);

    // Lighting Uniforms
    unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    unsigned int lightColorLoc =
        glGetUniformLocation(shaderProgram, "lightColor");

    // Light position at the top middle of the ceiling (Room is 30x30x30, so
    // ceiling is at y=15)
    glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
    glUniform3f(viewPosLoc, camera.Position[0], camera.Position[1],
                camera.Position[2]);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // White light

    // Draw Room

    // Wall Back - rgb(60, 99, 130) -> (0.235f, 0.388f, 0.510f)
    glUniform3f(colorLoc, 60.0f / 255.0f, 99.0f / 255.0f, 130.0f / 255.0f);
    room.drawWallBack();

    // Wall Front - rgb(96, 163, 188) -> (0.376f, 0.639f, 0.737f)
    // glUniform3f(colorLoc, 96.0f/255.0f, 163.0f/255.0f, 188.0f/255.0f);
    // room.drawWallFront();

    // Wall Left - rgb(10, 61, 98) -> (0.039f, 0.239f, 0.384f)
    glUniform3f(colorLoc, 10.0f / 255.0f, 61.0f / 255.0f, 98.0f / 255.0f);
    room.drawWallLeft();

    // Wall Right - rgb(106, 137, 204) -> (0.416f, 0.537f, 0.800f)
    glUniform3f(colorLoc, 106.0f / 255.0f, 137.0f / 255.0f, 204.0f / 255.0f);
    room.drawWallRight();

    // Floor - Lighter Gray
    glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
    room.drawFloor();

    // Ceiling - Dark Gray
    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
    room.drawCeiling();

    // Draw Kurna (Marble White/Grey)
    glUniform3f(colorLoc, 0.9f, 0.9f, 0.9f);
    glm_translate(
        model, (vec3){0.0f, -15.0f, 0.0f}); // [REVERTED] Floor is back at -15
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float *)model);
    kurna.draw();

    // [NEW] Render Particles as Points (DEBUG MODE)
    if (debugMode) {
      // Create VAO/VBO on first run (lazy init for simple structure)
      static unsigned int particleVAO = 0;
      static unsigned int particleVBO = 0;
      if (particleVAO == 0) {
        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        // initial buffer size big enough
        glBufferData(GL_ARRAY_BUFFER, 100000 * 3 * sizeof(float), NULL,
                     GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                              (void *)0);
        glBindVertexArray(0);
      }

      std::vector<float> particlePositions;
      for (const auto &p : steamEngine.getParticles()) {
        if (p.active) {
          particlePositions.push_back(p.position[0]);
          particlePositions.push_back(p.position[1]);
          particlePositions.push_back(p.position[2]);
        }
      }

      if (!particlePositions.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        // Ensure buffer is large enough if we exceeded initial guess (simple
        // realloc logic could go here) For now assuming 100k limit logic holds
        // or is sufficient for demo
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        particlePositions.size() * sizeof(float),
                        particlePositions.data());

        glUseProgram(shaderProgram);
        glm_mat4_identity(model);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float *)model);
        // Cyan color for steam particles
        glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f);

        glBindVertexArray(particleVAO);
        glPointSize(5.0f); // Make them visible
        glDrawArrays(GL_POINTS, 0, particlePositions.size() / 3);
        glBindVertexArray(0);
        glPointSize(1.0f); // Reset
      }
    }

    /* Volumetric Render Disabled for Particle Debug
    // [NEW] Draw Volumetric Steam
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(volShader);

    glm_mat4_identity(model);
    glm_scale(model, (vec3){15.0f, 15.0f, 15.0f}); // Scale to room size (radius
    15 -> 30 width)

    glUniformMatrix4fv(glGetUniformLocation(volShader, "model"), 1, GL_FALSE,
    (float *)model); glUniformMatrix4fv(glGetUniformLocation(volShader, "view"),
    1, GL_FALSE, (float *)view);
    glUniformMatrix4fv(glGetUniformLocation(volShader, "projection"), 1,
    GL_FALSE, (float *)projection);

    glUniform3f(glGetUniformLocation(volShader, "viewPos"), camera.Position[0],
    camera.Position[1], camera.Position[2]);
    glUniform3f(glGetUniformLocation(volShader, "boxMin"), -15.0f, -15.0f,
    -15.0f); glUniform3f(glGetUniformLocation(volShader,
    "boxMax"), 15.0f, 15.0f, 15.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, volTexture);
    glUniform1i(glGetUniformLocation(volShader, "densityTex"), 0);

    // Render Cube for Ray Marching Bounds
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    */

    // [NEW] Draw Volumetric Steam (If NOT Debug Mode)
    if (!debugMode) {
      // [TODO] Re-enable/Implement full volumetric path here if desired
      // For now, ensuring we don't crash or run empty shader code.
      // User requested toggle.
      // Logic for volumetric rendering:
      // 1. Update DensityVolume from particles
      densityVolume.Clear();
      for (const auto &p : steamEngine.getParticles()) {
        if (p.active) {
          // Map world pos to grid.
          // World: -15 to 15? Grid: 0 to 64
          // Need a mapping function/logic.
          // For now, let's just stick to the particles until DensityVolume is
          // fully connected.
        }
      }

      // Temporarily, let's keep the block commented out or minimal until
      // DensityVolume is verified connected to avoid "Red Screen" if texture is
      // empty. Or we can just enable the *code* but since densityVolume isn't
      // populated it might be invisible.

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glUseProgram(volShader);

      glm_mat4_identity(model);
      glm_scale(model,
                (vec3){25.0f, 15.0f, 25.0f}); // Scale to room size (50x30x50)

      glUniformMatrix4fv(glGetUniformLocation(volShader, "model"), 1, GL_FALSE,
                         (float *)model);
      glUniformMatrix4fv(glGetUniformLocation(volShader, "view"), 1, GL_FALSE,
                         (float *)view);
      glUniformMatrix4fv(glGetUniformLocation(volShader, "projection"), 1,
                         GL_FALSE, (float *)projection);

      glUniform3f(glGetUniformLocation(volShader, "viewPos"),
                  camera.Position[0], camera.Position[1], camera.Position[2]);
      glUniform3f(glGetUniformLocation(volShader, "boxMin"), -25.0f, -15.0f,
                  -25.0f);
      glUniform3f(glGetUniformLocation(volShader, "boxMax"), 25.0f, 15.0f,
                  25.0f);
      // Pass Light Pos (Top Center)
      glUniform3f(glGetUniformLocation(volShader, "lightPos"), 0.0f, 15.0f,
                  0.0f);

      // Pass Step Size
      glUniform1f(glGetUniformLocation(volShader, "stepSize"), rayStepSize);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, volTexture);
      glUniform1i(glGetUniformLocation(volShader, "densityTex"), 0);

      // Render Cube for Ray Marching Bounds
      glBindVertexArray(cubeVAO);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);

      glDisable(GL_BLEND);
    }

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // [NEW] Toggle Mouse with LEFT ALT
  static bool altPressed = false;
  if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
    if (!altPressed) {
      cameraEnabled = !cameraEnabled;
      if (cameraEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true; // Reset to avoid jump
      } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
      altPressed = true;
    }
  } else {
    altPressed = false;
  }

  // Only move camera if enabled
  if (cameraEnabled) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
      camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
      camera.ProcessKeyboard(DOWN, deltaTime);
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (!cameraEnabled)
    return; // [NEW] Stop camera look if mouse is free

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
  camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.ProcessMouseScroll(yoffset);
}

unsigned int createShader(const char *vertexPath, const char *fragmentPath) {
  std::string vertexCode;
  std::string fragmentCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;

  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;

    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    vShaderFile.close();
    fShaderFile.close();

    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
  } catch (std::ifstream::failure &e) {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
  }

  const char *vShaderCode = vertexCode.c_str();
  const char *fShaderCode = fragmentCode.c_str();

  unsigned int vertex, fragment;

  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);

  // Check compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);

  // Check compile errors
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  unsigned int ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);

  // Check link errors
  glGetProgramiv(ID, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(ID, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
              << infoLog << std::endl;
  }

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  return ID;
}
