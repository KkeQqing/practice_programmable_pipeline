//// SWE.cpp
//// 浅水波仿真（CPU 版本）+ 现代 OpenGL 渲染（无外部文件）
//// 依赖：GLFW, GLAD, OpenGL 3.3 Core
//// 编译（VS2022）：
//// cl /EHsc /std:c++17 /I"glfw/include" /I"glad/include" SWE.cpp /link glfw3.lib opengl32.lib
//
//#include <iostream>
//#include <vector>
//#include <cmath>
//#include <algorithm>
//
//#define GLFW_INCLUDE_NONE
//#include <GLFW/glfw3.h>
//#include <glad/glad.h>
//
//// -------------------------------
//// 水波模拟参数
//// -------------------------------
//const int GRID_WIDTH = 256;
//const int GRID_HEIGHT = 256;
//const float DX = 1.0f;          // 空间步长
//const float DT = 0.016f;        // 时间步长 (~60 FPS)
//const float GRAVITY = 9.81f;
//const float BASE_DEPTH = 1.0f;
//const float WAVE_SPEED = std::sqrt(GRAVITY * BASE_DEPTH);
//const float R = (WAVE_SPEED * DT) / DX;
//const float R_SQ = R * R;
//
//static_assert(R <= 0.5f, "Time step too large! Simulation will be unstable.");
//
//// -------------------------------
//// 水波场类（CPU）
//// -------------------------------
//class WaterSimulator {
//public:
//    WaterSimulator(int w, int h) : width(w), height(h) {
//        prev.assign(width * height, 0.0f);
//        curr.assign(width * height, 0.0f);
//        next.assign(width * height, 0.0f);
//
//        int cx = width / 2;
//        int cy = height / 2;
//        float radius = std::min(width, height) * 0.1f;
//        for (int j = 0; j < height; ++j) {
//            for (int i = 0; i < width; ++i) {
//                float dx = static_cast<float>(i - cx);
//                float dy = static_cast<float>(j - cy);
//                float dist = std::sqrt(dx * dx + dy * dy);
//                if (dist < radius) {
//                    float weight = 1.0f - (dist / radius);
//                    curr[j * width + i] = -0.5f * weight * weight;
//                }
//            }
//        }
//        prev = curr;
//    }
//
//    void step() {
//        for (int j = 1; j < height - 1; ++j) {
//            for (int i = 1; i < width - 1; ++i) {
//                float lap = curr[(j - 1) * width + i] +
//                    curr[(j + 1) * width + i] +
//                    curr[j * width + i - 1] +
//                    curr[j * width + i + 1] -
//                    4.0f * curr[j * width + i];
//                next[j * width + i] = 2.0f * curr[j * width + i] - prev[j * width + i] + R_SQ * lap;
//            }
//        }
//
//        // Neumann 边界（镜像）
//        for (int i = 0; i < width; ++i) {
//            next[i] = next[width + i];
//            next[(height - 1) * width + i] = next[(height - 2) * width + i];
//        }
//        for (int j = 0; j < height; ++j) {
//            next[j * width] = next[j * width + 1];
//            next[j * width + width - 1] = next[j * width + width - 2];
//        }
//
//        prev.swap(curr);
//        curr.swap(next);
//    }
//
//    const std::vector<float>& getHeightField() const { return curr; }
//
//private:
//    int width, height;
//    std::vector<float> prev, curr, next;
//};
//
//// -------------------------------
//// 创建着色器程序（内联 shader 源码）
//// -------------------------------
//unsigned int createShaderProgram() {
//    const char* vertexShaderSource = R"(
//        #version 330 core
//        layout (location = 0) in vec2 aPos;
//        out vec2 TexCoord;
//        void main() {
//            gl_Position = vec4(aPos, 0.0, 1.0);
//            TexCoord = aPos * 0.5 + 0.5; // 映射 [-1,1] → [0,1]
//        }
//    )";
//
//    const char* fragmentShaderSource = R"(
//        #version 330 core
//        in vec2 TexCoord;
//        out vec4 FragColor;
//        uniform sampler2D heightMap;
//        void main() {
//            float h = texture(heightMap, TexCoord).r;
//            FragColor = vec4(h, h, h, 1.0);
//        }
//    )";
//
//    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
//    glCompileShader(vertexShader);
//
//    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
//    glCompileShader(fragmentShader);
//
//    unsigned int program = glCreateProgram();
//    glAttachShader(program, vertexShader);
//    glAttachShader(program, fragmentShader);
//    glLinkProgram(program);
//
//    glDeleteShader(vertexShader);
//    glDeleteShader(fragmentShader);
//
//    return program;
//}
//
//// -------------------------------
//// 主程序
//// -------------------------------
//int main()
//{
//    if (!glfwInit()) {
//        std::cerr << "Failed to initialize GLFW\n";
//        return -1;
//    }
//
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//
//    GLFWwindow* window = glfwCreateWindow(800, 800, "Shallow Water Wave (CPU + Modern OpenGL)", nullptr, nullptr);
//    if (!window) {
//        std::cerr << "Failed to create GLFW window\n";
//        glfwTerminate();
//        return -1;
//    }
//    glfwMakeContextCurrent(window);
//
//    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//        std::cerr << "Failed to initialize GLAD\n";
//        return -1;
//    }
//
//    glEnable(GL_DEPTH_TEST);
//    glViewport(0, 0, 800, 800);
//
//    // 初始化水波模拟器
//    WaterSimulator sim(GRID_WIDTH, GRID_HEIGHT);
//
//    // 渲染缓冲（归一化到 [0,1]）
//    std::vector<float> render_buffer(GRID_WIDTH * GRID_HEIGHT);
//
//    // 创建全屏四边形（两个三角形）
//    float quadVertices[] = {
//        -1.0f, -1.0f,
//         1.0f, -1.0f,
//        -1.0f,  1.0f,
//         1.0f,  1.0f
//    };
//
//    unsigned int VAO, VBO;
//    glGenVertexArrays(1, &VAO);
//    glGenBuffers(1, &VBO);
//    glBindVertexArray(VAO);
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
//    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//    glBindVertexArray(0);
//
//    // 创建纹理
//    unsigned int texture;
//    glGenTextures(1, &texture);
//    glBindTexture(GL_TEXTURE_2D, texture);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glBindTexture(GL_TEXTURE_2D, 0);
//
//    // 创建着色器程序
//    unsigned int shaderProgram = createShaderProgram();
//    glUseProgram(shaderProgram);
//    glUniform1i(glGetUniformLocation(shaderProgram, "heightMap"), 0); // 绑定到纹理单元 0
//    glUseProgram(0);
//
//    while (!glfwWindowShouldClose(window))
//    {
//        // 模拟一步
//        sim.step();
//
//        // 准备渲染数据：归一化高度到 [0,1]
//        const auto& h = sim.getHeightField();
//        for (size_t i = 0; i < render_buffer.size(); ++i) {
//            render_buffer[i] = std::clamp(h[i] * 0.5f + 0.5f, 0.0f, 1.0f);
//        }
//
//        // 更新纹理
//        glBindTexture(GL_TEXTURE_2D, texture);
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, render_buffer.data());
//        glBindTexture(GL_TEXTURE_2D, 0);
//
//        // 渲染
//        glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        glUseProgram(shaderProgram);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, texture);
//        glBindVertexArray(VAO);
//        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//        glBindVertexArray(0);
//        glUseProgram(0);
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//
//    // 清理
//    glDeleteVertexArrays(1, &VAO);
//    glDeleteBuffers(1, &VBO);
//    glDeleteTextures(1, &texture);
//    glDeleteProgram(shaderProgram);
//
//    glfwDestroyWindow(window);
//    glfwTerminate();
//    return 0;
//}