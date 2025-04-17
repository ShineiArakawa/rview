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
      _shaderType(ImageShaderType::NEAREST),
      _imageShader(nullptr),
      _vao(),
      _vertexBuffer(QOpenGLBuffer::VertexBuffer),
      _indexBuffer(QOpenGLBuffer::IndexBuffer),
      _texture(nullptr),
      _glFunctions(nullptr),
      _isDragging(false) {
}

GLWidget::~GLWidget() {
  makeCurrent();

  _vertexBuffer.release();
  _indexBuffer.release();
  _vao.release();
  _texture->release();

  _texture->destroy();
  _vertexBuffer.destroy();
  _indexBuffer.destroy();
  _vao.destroy();

  delete _texture;

  doneCurrent();
}

void GLWidget::initializeGL() {
  // -----------------------------------------------------------------------------
  // Initialize OpenGL functions
  _glFunctions = QOpenGLContext::currentContext()->functions();
  if (_glFunctions == nullptr) {
    qDebug() << "Failed to get OpenGL functions";
    return;
  }

  _glFunctions->initializeOpenGLFunctions();

  // -----------------------------------------------------------------------------
  // Build the shader program

  _imageShader = std::make_shared<ImageShader>();

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

  // Set the vertex attribute pointers to all programs
  for (const auto &[type, program] : _imageShader->getShaderPrograms()) {
    program->bind();

    program->enableAttributeArray(0);
    program->enableAttributeArray(1);
    program->enableAttributeArray(2);

    program->release();
  }

  _glFunctions->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
  _glFunctions->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
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
  _texture->setMinificationFilter(QOpenGLTexture::Filter::NearestMipMapNearest);
  _texture->setMagnificationFilter(QOpenGLTexture::Filter::Nearest);
  _texture->setAutoMipMapGenerationEnabled(true);
  _texture->setWrapMode(QOpenGLTexture::ClampToEdge);
  _texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
  _texture->generateMipMaps();
  _texture->release();

  // -----------------------------------------------------------------------------
  // Initialize
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

  const auto &program = _imageShader->getShaderProgram(_shaderType);

  {
    program->bind();

    {
      // Activate the texture
      _texture->bind();

      // clang-format off
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_TEXTURE           , 0);
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_PIXEL_SIZE        , 1.0f / width(), 1.0f / height());
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_RECT_TOP_LEFT     , _rectTopLeft.x, _rectTopLeft.y);
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_RECT_BOTTOM_RIGHT , _rectBottomRight.x, _rectBottomRight.y);
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_BACKGROUND_COLOR  , _backgroundColor.r, _backgroundColor.g, _backgroundColor.b);
      program->setUniformValue(ImageShaderBase::UNIFORM_NAME_TEXTURE_SIZE      , _textureSize.x, _textureSize.y);
      // clang-format on

      _vao.bind();
      _glFunctions->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      _vao.release();

      _texture->release();
    }

    program->release();
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

  const float degrees = event->angleDelta().y() / 8.0f;
  const float steps = degrees / 15.0f;

  float scaleFactor = std::pow(1.1f, steps);

  const glm::vec2 currentSize = _rectBottomRight - _rectTopLeft;
  if (scaleFactor < 1.0f && (currentSize.x < 1.0 / width() || currentSize.y < 1.0 / height())) {
    scaleFactor = 1.0f;
  }

  const glm::vec2 center = (_rectBottomRight + _rectTopLeft) / 2.0f;

  _rectTopLeft = (_rectTopLeft - center) * scaleFactor;
  _rectBottomRight = (_rectBottomRight - center) * scaleFactor;

  // Translate the rectangle to the new center
  const glm::vec2 newCenter = (center - 0.5f) * scaleFactor + 0.5f;
  _rectTopLeft += newCenter;
  _rectBottomRight += newCenter;

  // Update the view
  update();

  event->accept();
}

void GLWidget::updateTexture(const cv::Mat &image) {
  if (image.empty()) {
    return;
  }

  {
    makeCurrent();

    // Resize the texture if necessary
    if (_textureSize.x != image.cols || _textureSize.y != image.rows) {
      _textureSize.x = image.cols;
      _textureSize.y = image.rows;

      _texture->destroy();
      _texture->create();

      _texture->bind();
      _texture->setFormat(QOpenGLTexture::RGBA32F);
      _texture->setSize(_textureSize.x, _textureSize.y);
      _texture->setMinificationFilter(QOpenGLTexture::Filter::NearestMipMapNearest);
      _texture->setMagnificationFilter(QOpenGLTexture::Filter::Nearest);
      _texture->setAutoMipMapGenerationEnabled(true);
      _texture->setWrapMode(QOpenGLTexture::ClampToEdge);
      _texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
      _texture->generateMipMaps();
      _texture->release();
    }

    resetRectPosition();

    // Upload the texture data
    _texture->bind();
    _texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, image.data);
    _texture->release();

    doneCurrent();
  }

  // Update the view
  update();
}

void GLWidget::setShaderType(ImageShaderType type) {
  _shaderType = type;

  // Update the view
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
