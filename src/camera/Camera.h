#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to
// stay away from window-system specific input methods
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 20.0f; // Updated default speed
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
  // Camera Attributes
  vec3 Position;
  vec3 Front;
  vec3 Up;
  vec3 Right;
  vec3 WorldUp;
  // Euler Angles
  float Yaw;
  float Pitch;
  // Camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;

  // Constructor with vectors
  Camera(vec3 position = (vec3){0.0f, 0.0f, 0.0f},
         vec3 up = (vec3){0.0f, 1.0f, 0.0f}, float yaw = YAW,
         float pitch = PITCH);

  // Constructor with scalar values
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
         float yaw, float pitch);

  // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
  void GetViewMatrix(mat4 dest);

  // Processes input received from any keyboard-like input system
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

#endif
