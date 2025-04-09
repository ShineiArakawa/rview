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

        out vec4 out_color;

        void main() {
          out_color = texture(u_texture, f_uv);
        }
    )";
};
