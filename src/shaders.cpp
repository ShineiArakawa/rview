#include <shaders.h>

ImageShader::ImageShader()
    : _shaderPrograms() {
  // Sure to call this constructor after making the context current

  for (const auto& shaderType : IMAGE_SHADER_TYPE_NAMES) {
    qDebug() << "Building shader program for type: " << QString::fromStdString(shaderType);

    const ImageShaderType type = stringToImageShaderType(shaderType);

    // Get the shader code for the specified type
    const auto& [vertexShaderCode, fragmentShaderCode] = getShaderCode(type);

    const std::shared_ptr<QOpenGLShaderProgram> program = std::make_shared<QOpenGLShaderProgram>();

    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderCode);
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderCode);

    program->link();

    _shaderPrograms[type] = program;

    qDebug() << "Done.";
  }
}

ImageShader::~ImageShader() {
  for (const auto& shaderProgram : _shaderPrograms) {
    shaderProgram.second->removeAllShaders();
    shaderProgram.second->release();
    shaderProgram.second->deleteLater();
  }
}

std::shared_ptr<QOpenGLShaderProgram> ImageShader::getShaderProgram(ImageShaderType type) const {
  auto it = _shaderPrograms.find(type);
  if (it != _shaderPrograms.end()) {
    return it->second;
  }

  throw std::invalid_argument("Invalid image shader type");

  return nullptr;
}