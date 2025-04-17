# RView

![teaser](/assets/teaser.png)

**RView** is a lightweight, high-performance image viewer designed for speed and responsiveness. It leverages OpenGL for fast rendering and uses multithreading for asynchronous image loading.

## Features

- 🚀 **Fast image rendering with OpenGL**
- 🔄 **Asynchronous image loading using multithreading**
- 🎨 **GLSL-based resampling filters**:
  - Nearest-neighbor
  - Bilinear
  - Bicubic
  - Lanczos4
- ⌨️ **Quick navigation between directories and images with arrow keys**

## Third Party Libraries

This project uses the following third-party libraries:

- [TinyEXIF](https://github.com/cdcseacave/TinyEXIF)  
  License: BSD-style license
- [tinyxml2](https://github.com/leethomason/tinyxml2)  
  License: zlib license
- [glm](https://github.com/g-truc/glm)  
  License: Happy Bunny License / MIT License
- [OpenCV](https://opencv.org/)  
  License: Apache License 2.0
- [Qt Framework](https://www.qt.io/)
  License: LGPL 3.0 License (dynamically linked)

Qt libraries are dynamically linked in accordance with the LGPL 3.0 license.  
You can obtain the source code of Qt from [Qt official website](https://download.qt.io/official_releases/qt/).

Each library's license terms are included in the [LICENSE.third_party](/LICENSE.third_party) file.

## Build

TO BE FILLED

## License

See [LICENCE](/LICENSE)
