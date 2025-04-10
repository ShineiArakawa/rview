#pragma once

class DefaultShader {
 public:
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

  inline static const char* FRAGMENT_SHADER_CODE = R"(
        #version 410 core

        in vec3 f_color;
        in vec2 f_uv;

        uniform sampler2D u_texture;
        uniform vec2 u_pixelSize;
        uniform vec2 u_rectTopLeft;
        uniform vec2 u_rectBottomRight;
        uniform vec3 u_backgroundColor;
        uniform vec2 u_textureSize;

        out vec4 out_color;

        vec2 toValidUV(vec2 uv) {
          return vec2(uv.x * u_textureSize.x, uv.y * u_textureSize.y);
        }

        void main() {
          out_color = vec4(u_backgroundColor, 1.0);

          if (f_uv.x >= u_rectTopLeft.x && f_uv.x <= u_rectBottomRight.x &&
              f_uv.y >= u_rectTopLeft.y && f_uv.y <= u_rectBottomRight.y) {
            vec2 uv = (f_uv - u_rectTopLeft) / (u_rectBottomRight - u_rectTopLeft); // Ranged in [0, 1]
            uv = toValidUV(uv);
            out_color = texture(u_texture, uv);
          }
        }
    )";
};
