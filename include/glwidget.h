#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QWheelEvent>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <opencv2/opencv.hpp>

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

// class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
class GLWidget : public QOpenGLWidget {
  Q_OBJECT

 public:
  inline static const glm::ivec2 TEXTURE_SIZE = glm::ivec2(12288, 12288);

  GLWidget(QWidget *parent = nullptr);
  ~GLWidget();

  void updateTexture(const cv::Mat &image);

 protected:
  void
  initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

 private:
  glm::ivec2 _textureSize;
  glm::vec2 _rectTopLeft;
  glm::vec2 _rectBottomRight;

  glm::vec3 _backgroundColor;

  QOpenGLShaderProgram *_program;

  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;
  QOpenGLBuffer _indexBuffer;
  QOpenGLTexture *_texture;

  QOpenGLFunctions *_glFunctions;

  bool _isDragging;
  glm::ivec2 _oldPos;
  glm::ivec2 _newPos;

  glm::ivec2 _oldWindowSize;

  void resetRectPosition();
};
