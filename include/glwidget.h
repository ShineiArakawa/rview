#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#define USE_FLOAT_TEXTURE

struct Vertex {
  Vertex() : position(0.0f), color(1.0f), uv(0.0f) {}
  Vertex(const glm::vec3 &position,
         const glm::vec3 &color,
         const glm::vec2 &uv)
      : position(position), color(color), uv(uv) {}

  glm::vec3 position;
  glm::vec3 color;
  glm::vec2 uv;
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

 public:
  GLWidget(QWidget *parent = nullptr);
  ~GLWidget();

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 private:
  QOpenGLShaderProgram *_program;

  GLuint _vaoId;
  GLuint _vertexBufferId;
  GLuint _indexBufferId;
  GLuint _textureId;
};
