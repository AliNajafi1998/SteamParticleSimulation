#ifndef KURNA_H
#define KURNA_H

#include <cmath>
#include <glad/glad.h>
#include <vector>

class Kurna {
public:
  Kurna(float radius, float height, int segments = 32);
  ~Kurna();

  void draw();

private:
  float radius, height;
  int segments;
  unsigned int VAO, VBO, EBO;
  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  void init();
  void buildCylinder();
};

#endif
