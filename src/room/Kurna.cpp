#include "Kurna.h"
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Kurna::Kurna(float radius, float height, int segments)
    : radius(radius), height(height), segments(segments), VAO(0), VBO(0),
      EBO(0) {
  init();
}

Kurna::~Kurna() {
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
}

void Kurna::init() {
  buildCylinder();

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Normal attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void Kurna::buildCylinder() {
  // Generate vertices
  // Top center
  // vertices.push_back(0.0f); vertices.push_back(height);
  // vertices.push_back(0.0f); // Pos vertices.push_back(0.0f);
  // vertices.push_back(1.0f); vertices.push_back(0.0f); // Normal

  // Bottom center
  // vertices.push_back(0.0f); vertices.push_back(0.0f);
  // vertices.push_back(0.0f); // Pos vertices.push_back(0.0f);
  // vertices.push_back(-1.0f); vertices.push_back(0.0f); // Normal

  // Side vertices
  for (int i = 0; i <= segments; ++i) {
    float theta = (float)i / (float)segments * 2.0f * M_PI;
    float x = radius * cos(theta);
    float z = radius * sin(theta);
    float nx = cos(theta);
    float nz = sin(theta);

    // Top edge vertex
    vertices.push_back(x);
    vertices.push_back(height);
    vertices.push_back(z);
    vertices.push_back(nx);
    vertices.push_back(0.0f);
    vertices.push_back(nz); // Side normal

    // Bottom edge vertex
    vertices.push_back(x);
    vertices.push_back(0.0f);
    vertices.push_back(z);
    vertices.push_back(nx);
    vertices.push_back(0.0f);
    vertices.push_back(nz); // Side normal
  }

  // Top cap vertices
  int baseTop = vertices.size() / 6;
  vertices.push_back(0.0f);
  vertices.push_back(height);
  vertices.push_back(0.0f); // Top center
  vertices.push_back(0.0f);
  vertices.push_back(1.0f);
  vertices.push_back(0.0f); // Normal Up

  for (int i = 0; i <= segments; ++i) {
    float theta = (float)i / (float)segments * 2.0f * M_PI;
    float x = radius * cos(theta);
    float z = radius * sin(theta);

    vertices.push_back(x);
    vertices.push_back(height);
    vertices.push_back(z);
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);
  }

  // Bottom cap vertices (not really needed if on floor, but good for
  // completeness)
  /* ... omitted for simplicity, can add if needed ... */

  // Indices for sides
  for (int i = 0; i < segments; ++i) {
    int i0 = i * 2;
    int i1 = i0 + 1;
    int i2 = (i + 1) * 2;
    int i3 = i2 + 1;

    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i2);
    indices.push_back(i1);
    indices.push_back(i3);
  }

  // Indices for Top Cap
  int topCenterIdx = baseTop;
  int firstCircleIdx = baseTop + 1;
  for (int i = 0; i < segments; ++i) {
    indices.push_back(topCenterIdx);
    indices.push_back(firstCircleIdx + i);
    indices.push_back(firstCircleIdx + i + 1);
  }
}

void Kurna::draw() {
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
