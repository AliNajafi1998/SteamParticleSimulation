#include "Camera.h"
#include <iostream>

// Constructor with vectors
Camera::Camera(vec3 position, vec3 up, float yaw, float pitch)
    : Front{0.0f, 0.0f, -1.0f}, MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  glm_vec3_copy(position, Position);
  glm_vec3_copy(up, WorldUp);
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY,
               float upZ, float yaw, float pitch)
    : Front{0.0f, 0.0f, -1.0f}, MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
  Position[0] = posX;
  Position[1] = posY;
  Position[2] = posZ;
  WorldUp[0] = upX;
  WorldUp[1] = upY;
  WorldUp[2] = upZ;
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
void Camera::GetViewMatrix(mat4 dest) {
  vec3 center;
  glm_vec3_add(Position, Front, center);
  glm_lookat(Position, center, Up, dest);
}

// Processes input received from any keyboard-like input system
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = MovementSpeed * deltaTime;
  vec3 temp;
  if (direction == FORWARD) {
    glm_vec3_scale(Front, velocity, temp);
    glm_vec3_add(Position, temp, Position);
  }
  if (direction == BACKWARD) {
    glm_vec3_scale(Front, velocity, temp);
    glm_vec3_sub(Position, temp, Position);
  }
  if (direction == LEFT) {
    glm_vec3_scale(Right, velocity, temp);
    glm_vec3_sub(Position, temp, Position);
  }
  if (direction == RIGHT) {
    glm_vec3_scale(Right, velocity, temp);
    glm_vec3_add(Position, temp, Position);
  }
  if (direction == UP) {
    glm_vec3_scale(WorldUp, velocity,
                   temp); // Using WorldUp for straightforward vertical movement
    glm_vec3_add(Position, temp, Position);
  }
  if (direction == DOWN) {
    glm_vec3_scale(WorldUp, velocity,
                   temp); // Using WorldUp for straightforward vertical movement
    glm_vec3_sub(Position, temp, Position);
  }
}

// Processes input received from a mouse input system
void Camera::ProcessMouseMovement(float xoffset, float yoffset,
                                  GLboolean constrainPitch) {
  xoffset *= MouseSensitivity;
  yoffset *= MouseSensitivity;

  Yaw += xoffset;
  Pitch += yoffset;

  if (constrainPitch) {
    if (Pitch > 89.0f)
      Pitch = 89.0f;
    if (Pitch < -89.0f)
      Pitch = -89.0f;
  }

  updateCameraVectors();
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors() {
  vec3 front;
  front[0] = cos(glm_rad(Yaw)) * cos(glm_rad(Pitch));
  front[1] = sin(glm_rad(Pitch));
  front[2] = sin(glm_rad(Yaw)) * cos(glm_rad(Pitch));
  glm_vec3_normalize_to(front, Front);

  vec3 right;
  glm_vec3_cross(Front, WorldUp, right);
  glm_vec3_normalize_to(right, Right);

  glm_vec3_cross(Right, Front, Up);
  glm_vec3_normalize(Up);
}

// Processes input received from a mouse scroll-wheel event. Only requires input
// on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset) {
  float velocity = MovementSpeed * yoffset * 0.1f;
  vec3 temp;
  glm_vec3_scale(Front, velocity, temp);
  glm_vec3_add(Position, temp, Position);
}
