#ifndef ROOM_H
#define ROOM_H

#include <glad/glad.h>
#include <vector>

class Room {
public:
  Room(float width, float height, float depth);
  ~Room();

  void draw();
  void drawWalls(); // Draws all walls at once
  void drawWallBack();
  void drawWallFront();
  void drawWallLeft();
  void drawWallRight();
  void drawFloor();
  void drawCeiling();

  void setTemperature(float t);
  float getTemperature() const;

private:
  float width, height, depth;
  float temperature;
  unsigned int VAO, VBO;
  std::vector<float> vertices;

  void init();
};

#endif
