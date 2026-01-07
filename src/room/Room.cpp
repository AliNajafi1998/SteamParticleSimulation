#include "Room.h"
#include <iostream>

Room::Room(float width, float height, float depth)
    : width(width), height(height), depth(depth), temperature(20.0f), VAO(0),
      VBO(0) {
  init();
}

Room::~Room() {
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
}

void Room::init() {
  float halfW = width / 2.0f;
  float halfH = height / 2.0f;
  float halfD = depth / 2.0f;

  // Define vertices for an inverted cube (visible from inside)
  // 36 vertices with 8 floats each (x, y, z, nx, ny, nz, u, v)
  vertices = {
      // Back face (Normal +Z)
      -halfW, -halfH, -halfD, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Bottom-left
      halfW, -halfH, -halfD, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // Bottom-right
      halfW, halfH, -halfD, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // Top-right
      halfW, halfH, -halfD, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // Top-right
      -halfW, halfH, -halfD, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // Top-left
      -halfW, -halfH, -halfD, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Bottom-left

      // Front face (Normal -Z)
      -halfW, -halfH, halfD, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, halfW, -halfH,
      halfD, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, halfW, halfH, halfD, 0.0f, 0.0f,
      -1.0f, 1.0f, 1.0f, halfW, halfH, halfD, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
      -halfW, halfH, halfD, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, -halfW, -halfH,
      halfD, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

      // Left face (Normal +X)
      -halfW, halfH, halfD, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, -halfW, halfH, -halfD,
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -halfW, -halfH, -halfD, 1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, -halfW, -halfH, -halfD, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -halfW,
      -halfH, halfD, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -halfW, halfH, halfD, 1.0f,
      0.0f, 0.0f, 1.0f, 0.0f,

      // Right face (Normal -X)
      halfW, halfH, halfD, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, halfW, halfH, -halfD,
      -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, halfW, -halfH, -halfD, -1.0f, 0.0f, 0.0f,
      1.0f, 1.0f, halfW, -halfH, -halfD, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, halfW,
      -halfH, halfD, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, halfW, halfH, halfD, -1.0f,
      0.0f, 0.0f, 0.0f, 0.0f,

      // Bottom face (Normal +Y) - Floor
      -halfW, -halfH, -halfD, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, halfW, -halfH,
      -halfD, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, halfW, -halfH, halfD, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, halfW, -halfH, halfD, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      -halfW, -halfH, halfD, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -halfW, -halfH,
      -halfD, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

      // Top face (Normal -Y) - Ceiling
      -halfW, halfH, -halfD, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, halfW, halfH,
      -halfD, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, halfW, halfH, halfD, 0.0f, -1.0f,
      0.0f, 1.0f, 0.0f, halfW, halfH, halfD, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
      -halfW, halfH, halfD, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -halfW, halfH,
      -halfD, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f};

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  // Vertex positions
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Normal vectors
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Texture coordinates
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0); // Unbind
}

void Room::draw() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void Room::drawWalls() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 24); // 4 walls * 6 vertices
  glBindVertexArray(0);
}

void Room::drawWallBack() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void Room::drawWallFront() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 6, 6);
  glBindVertexArray(0);
}

void Room::drawWallLeft() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 12, 6);
  glBindVertexArray(0);
}

void Room::drawWallRight() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 18, 6);
  glBindVertexArray(0);
}

void Room::drawFloor() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 24, 6); // Bottom face
  glBindVertexArray(0);
}

void Room::drawCeiling() {
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 30, 6); // Top face
  glBindVertexArray(0);
}

void Room::setTemperature(float t) { temperature = t; }

float Room::getTemperature() const { return temperature; }
