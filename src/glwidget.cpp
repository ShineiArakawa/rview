#include "glwidget.h"

#include <opencv2/opencv.hpp>
#include <vector>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      _program(nullptr),
      _vaoId(),
      _vertexBufferId(),
      _indexBufferId() {
}

GLWidget::~GLWidget() {
  makeCurrent();
  glDeleteBuffers(1, &_vertexBufferId);
  glDeleteBuffers(1, &_indexBufferId);
  glBindVertexArray(0);
  free(_program);
  doneCurrent();
}

void GLWidget::initializeGL() {
  initializeOpenGLFunctions();

  // -----------------------------------------------------------------------------
  // Create a quad
  const glm::vec3 baseColor(0.0f, 0.5f, 0.5f);
  const std::vector<Vertex> vertices = {
      // clang-format off
      Vertex(glm::vec3( -1.0f, -1.0f, 0.0f), baseColor, glm::vec2(0.0f, 0.0f)),
      Vertex(glm::vec3(  1.0f, -1.0f, 0.0f), baseColor, glm::vec2(1.0f, 0.0f)),
      Vertex(glm::vec3(  1.0f,  1.0f, 0.0f), baseColor, glm::vec2(1.0f, 1.0f)),
      Vertex(glm::vec3( -1.0f, -1.0f, 0.0f), baseColor, glm::vec2(0.0f, 0.0f)),
      Vertex(glm::vec3(  1.0f,  1.0f, 0.0f), baseColor, glm::vec2(1.0f, 1.0f)),
      Vertex(glm::vec3( -1.0f,  1.0f, 0.0f), baseColor, glm::vec2(0.0f, 1.0f)),
      // clang-format on
  };
  const std::vector<GLuint> indices = {
      0,
      1,
      2,
      3,
      4,
      5,
  };

  glGenVertexArrays(1, &_vaoId);
  glBindVertexArray(_vaoId);

  glGenBuffers(1, &_vertexBufferId);
  glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

  glGenBuffers(1, &_indexBufferId);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBufferId);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);

  // -----------------------------------------------------------------------------
  // Load the texture
  glEnable(GL_TEXTURE_2D);

  glGenTextures(1, &_textureId);
  glBindTexture(GL_TEXTURE_2D, _textureId);

#if defined(USE_FLOAT_TEXTURE)
  glTexImage2D(/* GLenum target */ GL_TEXTURE_2D,
               /* GLint level */ 0,
               /* GLint internalformat */ GL_RGBA32F,
               /* GLsizei width */ width(),
               /* GLsizei height */ height(),
               /* GLint border */ 0,
               /* GLenum format */ GL_RGBA,
               /* GLenum type */ GL_FLOAT,
               /* const GLvoid* pixels */ nullptr);
#else
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA8,
               width(),
               height(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               nullptr);
#endif

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(USE_FLOAT_TEXTURE)
  // Avoid mipmaps for float textures
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Avoid mipmaps for float textures
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D);
#endif

  glBindTexture(GL_TEXTURE_2D, 0);

  // -----------------------------------------------------------------------------
  // Build the shader program

  _program = new QOpenGLShaderProgram();
  _program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 410 core

        layout(location = 0) in vec3 in_position;
        layout(location = 1) in vec3 in_color;
        layout(location = 2) in vec2 in_uv;

        out vec3 f_color;
        out vec2 f_uv;

        void main() {
          f_color = in_color;
          f_uv = in_uv;
          gl_Position = vec4(in_position, 1.0);
        }
    )");
  _program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 410 core

        in vec3 f_color;
        in vec2 f_uv;

        uniform sampler2D u_texture;
        uniform vec2 u_pixelSize;

        out vec4 out_color;

        void main() {
          // out_color = texture(u_texture, f_uv);

          out_color = vec4(0.0, 0.0, 0.0, 1.0);

          // 角の1ピクセルだけ色を塗る
          if (
              (f_uv.x < u_pixelSize.x && f_uv.y < u_pixelSize.y) ||
              (f_uv.x > 1.0 - u_pixelSize.x && f_uv.y < u_pixelSize.y) ||
              (f_uv.x < u_pixelSize.x && f_uv.y > 1.0 - u_pixelSize.y) ||
              (f_uv.x > 1.0 - u_pixelSize.x && f_uv.y > 1.0 - u_pixelSize.y)
          ) {
            out_color = vec4(1.0, 1.0, 1.0, 1.0);
          }
        }
    )");

  _program->link();
  _program->bind();
}

void GLWidget::resizeGL(int w, int h) {
  glViewport(0, 0, w, h);

  // Update the texture size
  glBindTexture(GL_TEXTURE_2D, _textureId);
#if defined(USE_FLOAT_TEXTURE)
  glTexImage2D(/* GLenum target */ GL_TEXTURE_2D,
               /* GLint level */ 0,
               /* GLint internalformat */ GL_RGBA32F,
               /* GLsizei width */ w,
               /* GLsizei height */ h,
               /* GLint border */ 0,
               /* GLenum format */ GL_RGBA,
               /* GLenum type */ GL_FLOAT,
               /* const GLvoid* pixels */ nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Avoid mipmaps for float textures
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA8,
               w,
               h,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D);
#endif

  glBindTexture(GL_TEXTURE_2D, 0);
}

void GLWidget::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glDisable(GL_DEPTH_TEST);
  {
    _program->bind();
    {
      // Activate the texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _textureId);
      _program->setUniformValue("u_texture", 0);

      // Set the pixel size uniform
      _program->setUniformValue("u_pixelSize", 1.0f / width(), 1.0f / height());

      glBindVertexArray(_vaoId);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      glBindTexture(GL_TEXTURE_2D, 0);
    }
    _program->release();
  }
  glEnable(GL_DEPTH_TEST);
}

void GLWidget::mousePressEvent(QMouseEvent* event)  {
  qDebug() << "Pressed: " << event->button();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
qDebug() << "Moved: " << event->button();
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
qDebug() << "Released: " << event->button();
}
