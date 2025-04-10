#include <glwidget.h>
#include <shaders.h>

#include <vector>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      _textureSize(100, 100),
      _rectTopLeft(0.0f, 0.0f),
      _rectBottomRight(1.0f, 1.0f),
      _backgroundColor(0.1f, 0.1f, 0.1f),
      _oldWindowSize(0, 0),
      _vao(),
      _program(nullptr),
      _vertexBuffer(QOpenGLBuffer::VertexBuffer),
      _indexBuffer(QOpenGLBuffer::IndexBuffer),
      _texture(nullptr),
      _glFunctions(nullptr),
      _isDragging(false) {
}

GLWidget::~GLWidget() {
  makeCurrent();

  _program->release();
  _vertexBuffer.release();
  _indexBuffer.release();
  _vao.release();
  _texture->release();

  _program->deleteLater();
  _program->removeAllShaders();
  _texture->destroy();
  _vertexBuffer.destroy();
  _indexBuffer.destroy();
  _vao.destroy();

  delete _texture;
  delete _program;

  doneCurrent();
}

void GLWidget::initializeGL() {
  // ------------------------------------------------------------------------------------------
  // Initialize OpenGL functions
  _glFunctions = QOpenGLContext::currentContext()->functions();
  if (_glFunctions == nullptr) {
    qDebug() << "Failed to get OpenGL functions";
    return;
  }

  _glFunctions->initializeOpenGLFunctions();

  // -----------------------------------------------------------------------------
  // Build the shader program

  _program = new QOpenGLShaderProgram();

  _program->addShaderFromSourceCode(QOpenGLShader::Vertex, DefaultShader::VERTEX_SHADER_CODE);
  _program->addShaderFromSourceCode(QOpenGLShader::Fragment, DefaultShader::FRAGMENT_SHADER_CODE);

  _program->link();
  _program->bind();

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

  // -----------------------------------------------------------------------------
  // Create the vertex array object
  _vao.create();
  _vao.bind();

  // Vertex buffer object
  _vertexBuffer.create();
  _vertexBuffer.bind();

  _vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  _vertexBuffer.allocate(vertices.data(), sizeof(Vertex) * vertices.size());

  _program->enableAttributeArray(0);
  _glFunctions->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));

  _program->enableAttributeArray(1);
  _glFunctions->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));

  _program->enableAttributeArray(2);
  _glFunctions->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

  // Index buffer object
  _indexBuffer.create();
  _indexBuffer.bind();

  _indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  _indexBuffer.allocate(indices.data(), sizeof(uint32_t) * indices.size());

  // Unbind the buffers
  _vao.release();

  // -----------------------------------------------------------------------------
  // Load the texture
  _glFunctions->glEnable(GL_TEXTURE_2D);

  // https://www.badprog.com/c-qt-framework-using-opengl-vao-and-vbo-to-handle-2-different-objects-on-the-scene
  _texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
  _texture->create();
  _texture->bind();
  _texture->setFormat(QOpenGLTexture::RGBA32F);
  _texture->setSize(_textureSize.x, _textureSize.y);
  _texture->setMinificationFilter(QOpenGLTexture::Filter::Nearest);
  _texture->setMagnificationFilter(QOpenGLTexture::Filter::Nearest);
  _texture->setWrapMode(QOpenGLTexture::ClampToEdge);
  _texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);

  // Initialize the texture data
  const cv::Mat initialData(_textureSize.y, _textureSize.x, CV_32FC4, cv::Scalar(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, 1.0f));
  _texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, initialData.data);
  _texture->release();

  _oldWindowSize = glm::ivec2(width(), height());
}

void GLWidget::resizeGL(int w, int h) {
  const qreal retinaScale = devicePixelRatio();
  _glFunctions->glViewport(0, 0, w * retinaScale, h * retinaScale);

  resetRectPosition();
}

void GLWidget::paintGL() {
  const qreal retinaScale = devicePixelRatio();
  _glFunctions->glViewport(0, 0, width() * retinaScale, height() * retinaScale);

  _glFunctions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  _glFunctions->glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  _glFunctions->glDisable(GL_DEPTH_TEST);

  {
    _program->bind();

    {
      // Activate the texture
      _texture->bind();
      _program->setUniformValue("u_texture", 0);

      // Set the pixel size uniform
      _program->setUniformValue("u_pixelSize", 1.0f / width(), 1.0f / height());

      // Set the rectangle position uniform
      _program->setUniformValue("u_rectTopLeft", _rectTopLeft.x, _rectTopLeft.y);
      _program->setUniformValue("u_rectBottomRight", _rectBottomRight.x, _rectBottomRight.y);

      // Set the background color uniform
      _program->setUniformValue("u_backgroundColor", _backgroundColor.r, _backgroundColor.g, _backgroundColor.b);

      _vao.bind();
      _glFunctions->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      _vao.release();

      _texture->release();
    }

    _program->release();
  }

  _glFunctions->glEnable(GL_DEPTH_TEST);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
  if (event == nullptr) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    if (!_isDragging) {
      _isDragging = true;
      _oldPos = glm::ivec2(-event->pos().x(), event->pos().y());
      _newPos = _oldPos;
    }
  } else {
    _isDragging = false;
    _oldPos = glm::ivec2(0, 0);
    _newPos = glm::ivec2(0, 0);
  }

  event->accept();
}

void GLWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event == nullptr) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    resetRectPosition();
    update();
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event == nullptr) {
    return;
  }

  if (_isDragging) {
    _newPos = glm::ivec2(-event->pos().x(), event->pos().y());

    const glm::ivec2 deltaPix = _newPos - _oldPos;
    const float distSquared = deltaPix.x * deltaPix.x + deltaPix.y * deltaPix.y;

    if (distSquared >= 1.0f) {
      // Update the rectangle position based on the mouse movement
      const glm::vec2 delta = glm::vec2(deltaPix.x, deltaPix.y) / glm::vec2(width(), height());

      _rectTopLeft -= delta;
      _rectBottomRight -= delta;

      // Update the old position
      _oldPos = _newPos;

      // Update the OpenGL widget
      update();
    }
  }

  event->accept();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event == nullptr) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    _isDragging = false;
    _oldPos = glm::ivec2(0, 0);
    _newPos = glm::ivec2(0, 0);
  }

  event->accept();
}

void GLWidget::wheelEvent(QWheelEvent *event) {
  if (event == nullptr) {
    return;
  }

  // マウスホイールの回転量を取得（1ステップあたり15°、単位は度）
  const float degrees = event->angleDelta().y() / 8.0f;
  const float steps = degrees / 15.0f;

  // 各ステップごとに1.1倍/0.9倍で拡大縮小する（複数ステップの場合は累乗する）
  const float scaleFactor = std::pow(1.1f, steps);

  // 現在の画像表示領域の中心を計算
  const glm::vec2 center = (_rectBottomRight + _rectTopLeft) / 2.0f;
  const glm::vec2 windowCenter(0.5f, 0.5f);

  _rectTopLeft = (_rectTopLeft - center) * scaleFactor;
  _rectBottomRight = (_rectBottomRight - center) * scaleFactor;

  // 新しい中心を計算
  const glm::vec2 newCenter = (center - windowCenter) * scaleFactor + windowCenter;
  _rectTopLeft += newCenter;
  _rectBottomRight += newCenter;

  // OpenGLウィジェットを更新
  update();

  event->accept();
}

void GLWidget::updateTexture(const cv::Mat &image) {
  if (image.empty()) {
    return;
  }

  makeCurrent();

  // Convert the image to RGBA format
  cv::Mat rgbaImage;
  if (image.channels() == 1) {
    cv::cvtColor(image, rgbaImage, cv::COLOR_GRAY2RGBA);
  } else if (image.channels() == 3) {
    cv::cvtColor(image, rgbaImage, cv::COLOR_BGR2RGBA);
  } else if (image.channels() == 4) {
    rgbaImage = image;
  } else {
    qDebug() << "Unsupported image format";
    return;
  }

  // Convert the image to float32 format range [0, 1]
  double minVal, maxVal;
  cv::minMaxLoc(rgbaImage, &minVal, &maxVal);
  rgbaImage.convertTo(rgbaImage, CV_32F, 1.0 / (maxVal - minVal), -minVal / (maxVal - minVal));

  // Flip the image vertically
  cv::flip(rgbaImage, rgbaImage, 0);

  // Upload the texture data
  if (_textureSize.x != rgbaImage.cols || _textureSize.y != rgbaImage.rows) {
    _textureSize.x = rgbaImage.cols;
    _textureSize.y = rgbaImage.rows;

    _texture->destroy();
    _texture->create();

    _texture->bind();
    _texture->setFormat(QOpenGLTexture::RGBA32F);
    _texture->setSize(_textureSize.x, _textureSize.y);
    _texture->setMinificationFilter(QOpenGLTexture::Filter::Nearest);
    _texture->setMagnificationFilter(QOpenGLTexture::Filter::Nearest);
    _texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    _texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
    _texture->release();
  }

  resetRectPosition();

  _texture->bind();
  _texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, rgbaImage.data);
  _texture->release();

  doneCurrent();

  // Update
  update();
}

void GLWidget::resetRectPosition() {
  const glm::ivec2 windowSize(width(), height());

  const float textureAspect = static_cast<float>(_textureSize.x) / static_cast<float>(_textureSize.y);
  const float windowAspect = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);

  if (textureAspect > windowAspect) {
    // Texture is wider than window
    const float scale = static_cast<float>(windowSize.x) / static_cast<float>(_textureSize.x);
    const float width = scale * _textureSize.x;
    const float height = scale * _textureSize.y;

    _rectTopLeft.x = (windowSize.x - width) / 2.0f / windowSize.x;
    _rectTopLeft.y = (windowSize.y - height) / 2.0f / windowSize.y;
    _rectBottomRight.x = _rectTopLeft.x + width / windowSize.x;
    _rectBottomRight.y = _rectTopLeft.y + height / windowSize.y;
  } else {
    // Texture is taller than window
    const float scale = static_cast<float>(windowSize.y) / static_cast<float>(_textureSize.y);
    const float width = scale * _textureSize.x;
    const float height = scale * _textureSize.y;

    _rectTopLeft.x = (windowSize.x - width) / 2.0f / windowSize.x;
    _rectTopLeft.y = (windowSize.y - height) / 2.0f / windowSize.y;
    _rectBottomRight.x = _rectTopLeft.x + width / windowSize.x;
    _rectBottomRight.y = _rectTopLeft.y + height / windowSize.y;
  }
}
