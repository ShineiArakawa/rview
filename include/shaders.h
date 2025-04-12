#pragma once

#include <QOpenGLShaderProgram>
#include <QString>
#include <memory>
#include <string>
#include <vector>

// ###########################################################################################################################################
// Shader codes
// ###########################################################################################################################################
class DefaultVertexShader {
 public:
  virtual ~DefaultVertexShader() = default;

  QString getVertexShaderCode() const {
    return QString(VERTEX_SHADER_CODE);
  }

  inline static const char* VERTEX_SHADER_CODE = R"(
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
    )";
};

class ImageShaderBase : public DefaultVertexShader {
 protected:
  virtual const char* getFragmentShaderMainFuncCode() const = 0;

 public:
  virtual ~ImageShaderBase() = default;

  // clang-format off
  inline static const char* UNIFORM_NAME_TEXTURE           = "u_texture";
  inline static const char* UNIFORM_NAME_PIXEL_SIZE        = "u_pixelSize";
  inline static const char* UNIFORM_NAME_RECT_TOP_LEFT     = "u_rectTopLeft";
  inline static const char* UNIFORM_NAME_RECT_BOTTOM_RIGHT = "u_rectBottomRight";
  inline static const char* UNIFORM_NAME_BACKGROUND_COLOR  = "u_backgroundColor";
  inline static const char* UNIFORM_NAME_TEXTURE_SIZE      = "u_textureSize";
  // clang-format on

  inline static const char* FRAGMENT_SHADER_CODE_PRE = R"(
#version 410 core

in vec3 f_color;
in vec2 f_uv;

uniform sampler2D u_texture;
uniform vec2 u_pixelSize;
uniform vec2 u_rectTopLeft;
uniform vec2 u_rectBottomRight;
uniform vec3 u_backgroundColor;
uniform vec2 u_textureSize; // Texture size

out vec4 out_color;

  )";

  QString getFragmentShaderCode() const {
    // Assemble the fragment shader code
    QString fragmentShaderCode = FRAGMENT_SHADER_CODE_PRE;
    fragmentShaderCode += getFragmentShaderMainFuncCode();
    return fragmentShaderCode;
  }
};

class NearestImageShader : public ImageShaderBase {
 protected:
  const char* getFragmentShaderMainFuncCode() const override {
    return R"(
void main() {
  out_color = vec4(u_backgroundColor, 1.0);

  if (f_uv.x >= u_rectTopLeft.x && f_uv.x <= u_rectBottomRight.x &&
      f_uv.y >= u_rectTopLeft.y && f_uv.y <= u_rectBottomRight.y) {
    vec2 uv = (f_uv - u_rectTopLeft) / (u_rectBottomRight - u_rectTopLeft); // Ranged in [0, 1]

    // Calc the pixel index of nearest pixel
    // Note: The pixel index is calculated based on the texture size
    int pixelIdxX = int(uv.x * float(u_textureSize.x));
    int pixelIdxY = int(uv.y * float(u_textureSize.y));

    if (pixelIdxX == int(u_textureSize.x)) {
      pixelIdxX = int(u_textureSize.x) - 1;
    }
    if (pixelIdxY == int(u_textureSize.y)) {
      pixelIdxY = int(u_textureSize.y) - 1;
    }

    // Calc the pixel coordinates
    float pixelX = (float(pixelIdxX) + 0.5) / float(u_textureSize.x);
    float pixelY = (float(pixelIdxY) + 0.5) / float(u_textureSize.y);

    // Calc the color
    out_color = texture(u_texture, vec2(pixelX, pixelY));
  }
}
  )";
  }
};

class BilinearImageShader : public ImageShaderBase {
 protected:
  const char* getFragmentShaderMainFuncCode() const override {
    return R"(
void main() {
  out_color = vec4(u_backgroundColor, 1.0);

  if (f_uv.x >= u_rectTopLeft.x && f_uv.x <= u_rectBottomRight.x &&
      f_uv.y >= u_rectTopLeft.y && f_uv.y <= u_rectBottomRight.y) {
    vec2 uv = (f_uv - u_rectTopLeft) / (u_rectBottomRight - u_rectTopLeft); // Ranged in [0, 1]
    
    float strideX = 1.0 / float(u_textureSize.x);
    float strideY = 1.0 / float(u_textureSize.y);

    // Calc the pixel index of nearest Left-top pixel
    int pixelIdxX = int((uv.x - strideX / 2.0) / strideX); // Ranged in [-1, u_textureSize.x - 1]
    int pixelIdxY = int((uv.y - strideY / 2.0) / strideY); // Ranged in [-1, u_textureSize.y - 1]

    // Calc the pixel coordinates
    float pixelXLow = (float(pixelIdxX) + 0.5) * strideX;
    float pixelYLow = (float(pixelIdxY) + 0.5) * strideY;
    float pixelXHigh = pixelXLow + strideX;
    float pixelYHigh = pixelYLow + strideY;

    // Calc weights
    float alphaX = (uv.x - pixelXLow) / strideX;
    float alphaY = (uv.y - pixelYLow) / strideY;

    float weightUL = (1.0 - alphaX) * (1.0 - alphaY);
    float weightUR = alphaX * (1.0 - alphaY);
    float weightLL = (1.0 - alphaX) * alphaY;
    float weightLR = alphaX * alphaY;

    // Calc the color
    float validPixelXLow = max(0.0, min(1.0, pixelXLow));
    float validPixelXHigh = max(0.0, min(1.0, pixelXHigh));
    float validPixelYLow = max(0.0, min(1.0, pixelYLow));
    float validPixelYHigh = max(0.0, min(1.0, pixelYHigh));

    vec4 colorUL = texture(u_texture, vec2(validPixelXLow, validPixelYLow));
    vec4 colorUR = texture(u_texture, vec2(validPixelXHigh, validPixelYLow));
    vec4 colorLL = texture(u_texture, vec2(validPixelXLow, validPixelYHigh));
    vec4 colorLR = texture(u_texture, vec2(validPixelXHigh, validPixelYHigh));

    out_color = weightUL * colorUL + weightUR * colorUR +
                weightLL * colorLL + weightLR * colorLR;
    out_color.r = clamp(out_color.r, 0.0, 1.0);
    out_color.g = clamp(out_color.g, 0.0, 1.0);
    out_color.b = clamp(out_color.b, 0.0, 1.0);
    out_color.a = 1.0;
  }
}
  )";
  }
};

class BicubicImageShader : public ImageShaderBase {
 protected:
  const char* getFragmentShaderMainFuncCode() const override {
    return R"(
float sinc_taylor_expanded(float x, float a) {
  float x_abs = abs(x);
  float res = 0.0;

  if (x_abs <= 1.0) {
    float x_abs_2 = x_abs * x_abs;
    float x_abs_3 = x_abs_2 * x_abs;
    res = (a + 2.0) * x_abs_3 - (a + 3.0) * x_abs_2 + 1.0;
  } else if (x_abs <= 2.0) {
    float x_abs_2 = x_abs * x_abs;
    float x_abs_3 = x_abs_2 * x_abs;
    res = a * x_abs_3 - 5.0 * a * x_abs_2 + 8.0 * a * x_abs - 4.0 * a;
  }

  return res;
}

void main() {
  out_color = vec4(u_backgroundColor, 1.0);

  if (f_uv.x >= u_rectTopLeft.x && f_uv.x <= u_rectBottomRight.x &&
      f_uv.y >= u_rectTopLeft.y && f_uv.y <= u_rectBottomRight.y) {
    vec2 uv = (f_uv - u_rectTopLeft) / (u_rectBottomRight - u_rectTopLeft); // Ranged in [0, 1]
    
    float strideX = 1.0 / float(u_textureSize.x);
    float strideY = 1.0 / float(u_textureSize.y);

    // Calc the pixel index of nearest Left-top pixel
    int pixelIdxX = int((uv.x - strideX / 2.0) / strideX);
    int pixelIdxY = int((uv.y - strideY / 2.0) / strideY);

    // Calc the pixel coordinates
    float pixelX1 = float(pixelIdxX) * strideX + strideX / 2.0;
    float pixelX0 = pixelX1 - strideX;
    float pixelX2 = pixelX1 + strideX;
    float pixelX3 = pixelX2 + strideX;
    float pixelY1 = float(pixelIdxY) * strideY + strideY / 2.0;
    float pixelY0 = pixelY1 - strideY;
    float pixelY2 = pixelY1 + strideY;
    float pixelY3 = pixelY2 + strideY;

    // Calc distance
    float distX0 = uv.x - pixelX0;
    float distX1 = uv.x - pixelX1;
    float distX2 = pixelX2 - uv.x;
    float distX3 = pixelX3 - uv.x;

    float distY0 = uv.y - pixelY0;
    float distY1 = uv.y - pixelY1;
    float distY2 = pixelY2 - uv.y;
    float distY3 = pixelY3 - uv.y;

    // Calc weights
    float a = -0.5;
    vec4 weightX = vec4(sinc_taylor_expanded(distX0 / strideX, a),
                        sinc_taylor_expanded(distX1 / strideX, a),
                        sinc_taylor_expanded(distX2 / strideX, a),
                        sinc_taylor_expanded(distX3 / strideX, a));
    vec4 weightY = vec4(sinc_taylor_expanded(distY0 / strideY, a),
                        sinc_taylor_expanded(distY1 / strideY, a),
                        sinc_taylor_expanded(distY2 / strideY, a),
                        sinc_taylor_expanded(distY3 / strideY, a));

    // Calc the color
    float validPixelX0 = max(0.0, min(1.0, pixelX0));
    float validPixelX1 = max(0.0, min(1.0, pixelX1));
    float validPixelX2 = max(0.0, min(1.0, pixelX2));
    float validPixelX3 = max(0.0, min(1.0, pixelX3));

    float validPixelY0 = max(0.0, min(1.0, pixelY0));
    float validPixelY1 = max(0.0, min(1.0, pixelY1));
    float validPixelY2 = max(0.0, min(1.0, pixelY2));
    float validPixelY3 = max(0.0, min(1.0, pixelY3));

    vec4 colorX0Y0 = texture(u_texture, vec2(validPixelX0, validPixelY0));
    vec4 colorX1Y0 = texture(u_texture, vec2(validPixelX1, validPixelY0));
    vec4 colorX2Y0 = texture(u_texture, vec2(validPixelX2, validPixelY0));
    vec4 colorX3Y0 = texture(u_texture, vec2(validPixelX3, validPixelY0));

    vec4 colorX0Y1 = texture(u_texture, vec2(validPixelX0, validPixelY1));
    vec4 colorX1Y1 = texture(u_texture, vec2(validPixelX1, validPixelY1));
    vec4 colorX2Y1 = texture(u_texture, vec2(validPixelX2, validPixelY1));
    vec4 colorX3Y1 = texture(u_texture, vec2(validPixelX3, validPixelY1));
    
    vec4 colorX0Y2 = texture(u_texture, vec2(validPixelX0, validPixelY2));
    vec4 colorX1Y2 = texture(u_texture, vec2(validPixelX1, validPixelY2));
    vec4 colorX2Y2 = texture(u_texture, vec2(validPixelX2, validPixelY2));
    vec4 colorX3Y2 = texture(u_texture, vec2(validPixelX3, validPixelY2));
    
    vec4 colorX0Y3 = texture(u_texture, vec2(validPixelX0, validPixelY3));
    vec4 colorX1Y3 = texture(u_texture, vec2(validPixelX1, validPixelY3));
    vec4 colorX2Y3 = texture(u_texture, vec2(validPixelX2, validPixelY3));
    vec4 colorX3Y3 = texture(u_texture, vec2(validPixelX3, validPixelY3));
    
    // Calc color * weightX
    vec4 tmpColor0 = colorX0Y0 * weightX.x + colorX1Y0 * weightX.y + colorX2Y0 * weightX.z + colorX3Y0 * weightX.w;
    vec4 tmpColor1 = colorX0Y1 * weightX.x + colorX1Y1 * weightX.y + colorX2Y1 * weightX.z + colorX3Y1 * weightX.w;
    vec4 tmpColor2 = colorX0Y2 * weightX.x + colorX1Y2 * weightX.y + colorX2Y2 * weightX.z + colorX3Y2 * weightX.w;
    vec4 tmpColor3 = colorX0Y3 * weightX.x + colorX1Y3 * weightX.y + colorX2Y3 * weightX.z + colorX3Y3 * weightX.w;

    // Calc weightY * color
    out_color = tmpColor0 * weightY.x + tmpColor1 * weightY.y + tmpColor2 * weightY.z + tmpColor3 * weightY.w;

    out_color.r = clamp(out_color.r, 0.0, 1.0);
    out_color.g = clamp(out_color.g, 0.0, 1.0);
    out_color.b = clamp(out_color.b, 0.0, 1.0);
    out_color.a = 1.0;
  }
}
  )";
  }
};

class Lanczos4ImageShader : public ImageShaderBase {
 protected:
  const char* getFragmentShaderMainFuncCode() const override {
    return R"(
#define PI 3.1415926525

float ideal_sinc(float x) {
  x = abs(x) + 1e-14;
  return sin(x) / x;
}

float calc_lanczos4_weight(float x) {
  float x_abs = abs(x);

  if (x_abs < 1e-14) {
    return 1.0;
  } else if (x_abs <= 4.0) {
    float y = PI * x_abs;
    return ideal_sinc(y) * ideal_sinc(y / 4.0);
  }

  return 0.0;
}

void main() {
  out_color = vec4(u_backgroundColor, 1.0);

  if (f_uv.x >= u_rectTopLeft.x && f_uv.x <= u_rectBottomRight.x &&
      f_uv.y >= u_rectTopLeft.y && f_uv.y <= u_rectBottomRight.y) {
    vec2 uv = (f_uv - u_rectTopLeft) / (u_rectBottomRight - u_rectTopLeft); // Ranged in [0, 1]
    
    float strideX = 1.0 / float(u_textureSize.x);
    float strideY = 1.0 / float(u_textureSize.y);

    // Calc the pixel index of nearest Left-top pixel
    int pixelIdxX = int((uv.x - strideX / 2.0) / strideX);
    int pixelIdxY = int((uv.y - strideY / 2.0) / strideY);

    // Calc the pixel coordinates (8 pixels per axis)
    float pixelX3 = float(pixelIdxX) * strideX + strideX / 2.0;
    float pixelX2 = pixelX3 - strideX;
    float pixelX1 = pixelX2 - strideX;
    float pixelX0 = pixelX1 - strideX;
    float pixelX4 = pixelX3 + strideX;
    float pixelX5 = pixelX4 + strideX;
    float pixelX6 = pixelX5 + strideX;
    float pixelX7 = pixelX6 + strideX;
    float pixelY3 = float(pixelIdxY) * strideY + strideY / 2.0;
    float pixelY2 = pixelY3 - strideY;
    float pixelY1 = pixelY2 - strideY;
    float pixelY0 = pixelY1 - strideY;
    float pixelY4 = pixelY3 + strideY;
    float pixelY5 = pixelY4 + strideY;
    float pixelY6 = pixelY5 + strideY;
    float pixelY7 = pixelY6 + strideY;

    // Calc distance
    float distX0 = uv.x - pixelX0;
    float distX1 = uv.x - pixelX1;
    float distX2 = uv.x - pixelX2;
    float distX3 = uv.x - pixelX3;
    float distX4 = pixelX4 - uv.x;
    float distX5 = pixelX5 - uv.x;
    float distX6 = pixelX6 - uv.x;
    float distX7 = pixelX7 - uv.x;

    float distY0 = uv.y - pixelY0;
    float distY1 = uv.y - pixelY1;
    float distY2 = uv.y - pixelY2;
    float distY3 = uv.y - pixelY3;
    float distY4 = pixelY4 - uv.y;
    float distY5 = pixelY5 - uv.y;
    float distY6 = pixelY6 - uv.y;
    float distY7 = pixelY7 - uv.y;

    // Calc weights
    float weightX0 = calc_lanczos4_weight(distX0 / strideX);
    float weightX1 = calc_lanczos4_weight(distX1 / strideX);
    float weightX2 = calc_lanczos4_weight(distX2 / strideX);
    float weightX3 = calc_lanczos4_weight(distX3 / strideX);
    float weightX4 = calc_lanczos4_weight(distX4 / strideX);
    float weightX5 = calc_lanczos4_weight(distX5 / strideX);
    float weightX6 = calc_lanczos4_weight(distX6 / strideX);
    float weightX7 = calc_lanczos4_weight(distX7 / strideX);

    float weightY0 = calc_lanczos4_weight(distY0 / strideY);
    float weightY1 = calc_lanczos4_weight(distY1 / strideY);
    float weightY2 = calc_lanczos4_weight(distY2 / strideY);
    float weightY3 = calc_lanczos4_weight(distY3 / strideY);
    float weightY4 = calc_lanczos4_weight(distY4 / strideY);
    float weightY5 = calc_lanczos4_weight(distY5 / strideY);
    float weightY6 = calc_lanczos4_weight(distY6 / strideY);
    float weightY7 = calc_lanczos4_weight(distY7 / strideY);

    // Calc valid pixel coordinates
    float x0 = max(0.0, min(1.0, pixelX0));
    float x1 = max(0.0, min(1.0, pixelX1));
    float x2 = max(0.0, min(1.0, pixelX2));
    float x3 = max(0.0, min(1.0, pixelX3));
    float x4 = max(0.0, min(1.0, pixelX4));
    float x5 = max(0.0, min(1.0, pixelX5));
    float x6 = max(0.0, min(1.0, pixelX6));
    float x7 = max(0.0, min(1.0, pixelX7));

    float y0 = max(0.0, min(1.0, pixelY0));
    float y1 = max(0.0, min(1.0, pixelY1));
    float y2 = max(0.0, min(1.0, pixelY2));
    float y3 = max(0.0, min(1.0, pixelY3));
    float y4 = max(0.0, min(1.0, pixelY4));
    float y5 = max(0.0, min(1.0, pixelY5));
    float y6 = max(0.0, min(1.0, pixelY6));
    float y7 = max(0.0, min(1.0, pixelY7));

    // Calc color * weightX
    vec4 tmpColor0 = texture(u_texture, vec2(x0, y0)) * weightX0 + texture(u_texture, vec2(x1, y0)) * weightX1 + texture(u_texture, vec2(x2, y0)) * weightX2 + texture(u_texture, vec2(x3, y0)) * weightX3 + texture(u_texture, vec2(x4, y0)) * weightX4 + texture(u_texture, vec2(x5, y0)) * weightX5 + texture(u_texture, vec2(x6, y0)) * weightX6 + texture(u_texture, vec2(x7, y0)) * weightX7;
    vec4 tmpColor1 = texture(u_texture, vec2(x0, y1)) * weightX0 + texture(u_texture, vec2(x1, y1)) * weightX1 + texture(u_texture, vec2(x2, y1)) * weightX2 + texture(u_texture, vec2(x3, y1)) * weightX3 + texture(u_texture, vec2(x4, y1)) * weightX4 + texture(u_texture, vec2(x5, y1)) * weightX5 + texture(u_texture, vec2(x6, y1)) * weightX6 + texture(u_texture, vec2(x7, y1)) * weightX7;
    vec4 tmpColor2 = texture(u_texture, vec2(x0, y2)) * weightX0 + texture(u_texture, vec2(x1, y2)) * weightX1 + texture(u_texture, vec2(x2, y2)) * weightX2 + texture(u_texture, vec2(x3, y2)) * weightX3 + texture(u_texture, vec2(x4, y2)) * weightX4 + texture(u_texture, vec2(x5, y2)) * weightX5 + texture(u_texture, vec2(x6, y2)) * weightX6 + texture(u_texture, vec2(x7, y2)) * weightX7;
    vec4 tmpColor3 = texture(u_texture, vec2(x0, y3)) * weightX0 + texture(u_texture, vec2(x1, y3)) * weightX1 + texture(u_texture, vec2(x2, y3)) * weightX2 + texture(u_texture, vec2(x3, y3)) * weightX3 + texture(u_texture, vec2(x4, y3)) * weightX4 + texture(u_texture, vec2(x5, y3)) * weightX5 + texture(u_texture, vec2(x6, y3)) * weightX6 + texture(u_texture, vec2(x7, y3)) * weightX7;
    vec4 tmpColor4 = texture(u_texture, vec2(x0, y4)) * weightX0 + texture(u_texture, vec2(x1, y4)) * weightX1 + texture(u_texture, vec2(x2, y4)) * weightX2 + texture(u_texture, vec2(x3, y4)) * weightX3 + texture(u_texture, vec2(x4, y4)) * weightX4 + texture(u_texture, vec2(x5, y4)) * weightX5 + texture(u_texture, vec2(x6, y4)) * weightX6 + texture(u_texture, vec2(x7, y4)) * weightX7;
    vec4 tmpColor5 = texture(u_texture, vec2(x0, y5)) * weightX0 + texture(u_texture, vec2(x1, y5)) * weightX1 + texture(u_texture, vec2(x2, y5)) * weightX2 + texture(u_texture, vec2(x3, y5)) * weightX3 + texture(u_texture, vec2(x4, y5)) * weightX4 + texture(u_texture, vec2(x5, y5)) * weightX5 + texture(u_texture, vec2(x6, y5)) * weightX6 + texture(u_texture, vec2(x7, y5)) * weightX7;
    vec4 tmpColor6 = texture(u_texture, vec2(x0, y6)) * weightX0 + texture(u_texture, vec2(x1, y6)) * weightX1 + texture(u_texture, vec2(x2, y6)) * weightX2 + texture(u_texture, vec2(x3, y6)) * weightX3 + texture(u_texture, vec2(x4, y6)) * weightX4 + texture(u_texture, vec2(x5, y6)) * weightX5 + texture(u_texture, vec2(x6, y6)) * weightX6 + texture(u_texture, vec2(x7, y6)) * weightX7;
    vec4 tmpColor7 = texture(u_texture, vec2(x0, y7)) * weightX0 + texture(u_texture, vec2(x1, y7)) * weightX1 + texture(u_texture, vec2(x2, y7)) * weightX2 + texture(u_texture, vec2(x3, y7)) * weightX3 + texture(u_texture, vec2(x4, y7)) * weightX4 + texture(u_texture, vec2(x5, y7)) * weightX5 + texture(u_texture, vec2(x6, y7)) * weightX6 + texture(u_texture, vec2(x7, y7)) * weightX7;

    // Calc weightY * color
    out_color = tmpColor0 * weightY0 +
                tmpColor1 * weightY1 +
                tmpColor2 * weightY2 +
                tmpColor3 * weightY3 +
                tmpColor4 * weightY4 +
                tmpColor5 * weightY5 +
                tmpColor6 * weightY6 +
                tmpColor7 * weightY7;

    // Clamp the color values to [0, 1]
    out_color.r = clamp(out_color.r, 0.0, 1.0);
    out_color.g = clamp(out_color.g, 0.0, 1.0);
    out_color.b = clamp(out_color.b, 0.0, 1.0);
    out_color.a = 1.0;
  }
}
  )";
  }
};

// ###########################################################################################################################################
// Image shader type
// ###########################################################################################################################################
inline static const std::vector<std::string> IMAGE_SHADER_TYPE_NAMES = {"Nearest",
                                                                        "Bilinear",
                                                                        "Bicubic",
                                                                        "Lanczos4"};

enum class ImageShaderType {
  NEAREST,
  BILINEAR,
  BICUBIC,
  LANCZOS4
};

inline static std::string imageShaderTypeToString(ImageShaderType type) {
  return IMAGE_SHADER_TYPE_NAMES[static_cast<int>(type)];
}

inline static ImageShaderType stringToImageShaderType(const std::string& type) {
  auto it = std::find(IMAGE_SHADER_TYPE_NAMES.begin(), IMAGE_SHADER_TYPE_NAMES.end(), type);
  if (it != IMAGE_SHADER_TYPE_NAMES.end()) {
    return static_cast<ImageShaderType>(std::distance(IMAGE_SHADER_TYPE_NAMES.begin(), it));
  }
  throw std::invalid_argument("Invalid image shader type: " + type);
}

inline static std::pair<QString, QString> getShaderCode(ImageShaderType type) {
  std::shared_ptr<ImageShaderBase> shader;

  switch (type) {
    case ImageShaderType::NEAREST:
      shader = std::make_shared<NearestImageShader>();
      break;
    case ImageShaderType::BILINEAR:
      shader = std::make_shared<BilinearImageShader>();
      break;
    case ImageShaderType::BICUBIC:
      shader = std::make_shared<BicubicImageShader>();
      break;
    case ImageShaderType::LANCZOS4:
      shader = std::make_shared<Lanczos4ImageShader>();
      break;
    default:
      throw std::invalid_argument("Invalid image shader type");
  }

  return {shader->getVertexShaderCode(), shader->getFragmentShaderCode()};
}

// ###########################################################################################################################################
// Shader manager
// ###########################################################################################################################################

class ImageShader {
 private:
  std::map<ImageShaderType, std::shared_ptr<QOpenGLShaderProgram>> _shaderPrograms;

 public:
  ImageShader();
  ~ImageShader();

  std::shared_ptr<QOpenGLShaderProgram> getShaderProgram(ImageShaderType type) const;
  std::map<ImageShaderType, std::shared_ptr<QOpenGLShaderProgram>> getShaderPrograms() const { return _shaderPrograms; }
};
