// SWE.cpp
// 浅水波仿真（CPU 版本）：基于波动方程的水波纹模拟
// 依赖：GLFW, GLAD, OpenGL
// 编译（Windows + VS2022）：
// cl /EHsc /I"glfw/include" /I"glad/include" SWE.cpp /link glfw3.lib opengl32.lib

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

// -------------------------------
// 水波模拟参数
// -------------------------------
const int GRID_WIDTH = 256;
const int GRID_HEIGHT = 256;
const float DX = 1.0f;          // 空间步长
const float DT = 0.016f;        // 时间步长 (~60 FPS)
const float GRAVITY = 9.81f;
const float BASE_DEPTH = 1.0f;  // 静水深度
const float WAVE_SPEED = std::sqrt(GRAVITY * BASE_DEPTH); // c = sqrt(g * h0)
const float R = (WAVE_SPEED * DT) / DX;
const float R_SQ = R * R;

// 确保稳定性：R <= 0.5（CFL 条件）
static_assert(R <= 0.5f, "Time step too large! Simulation will be unstable.");

// -------------------------------
// 水波场类（CPU）
// -------------------------------
class WaterSimulator {
public:
    WaterSimulator(int w, int h) : width(w), height(h) {
        prev.assign(width * height, 0.0f);
        curr.assign(width * height, 0.0f);
        next.assign(width * height, 0.0f);

        // 初始扰动：中心水滴
        int cx = width / 2;
        int cy = height / 2;
        float radius = std::min(width, height) * 0.1f;
        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                float dx = static_cast<float>(i - cx);
                float dy = static_cast<float>(j - cy);
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < radius) {
                    float weight = 1.0f - (dist / radius);
                    curr[j * width + i] = -0.5f * weight * weight; // 向下扰动
                }
            }
        }
        prev = curr; // 初始化 prev = curr
    }

    void step() {
        // 内部点更新（波动方程）
        for (int j = 1; j < height - 1; ++j) {
            for (int i = 1; i < width - 1; ++i) {
                float lap = curr[(j - 1) * width + i] +
                    curr[(j + 1) * width + i] +
                    curr[j * width + i - 1] +
                    curr[j * width + i + 1] -
                    4.0f * curr[j * width + i];

                next[j * width + i] = 2.0f * curr[j * width + i] - prev[j * width + i] + R_SQ * lap;
            }
        }

        // 边界条件：Neumann（镜像）
        for (int i = 0; i < width; ++i) {
            next[i] = next[width + i];                     // y=0
            next[(height - 1) * width + i] = next[(height - 2) * width + i]; // y=height-1
        }
        for (int j = 0; j < height; ++j) {
            next[j * width] = next[j * width + 1];         // x=0
            next[j * width + width - 1] = next[j * width + width - 2]; // x=width-1
        }

        // 交换缓冲区
        prev.swap(curr);
        curr.swap(next);
    }

    const std::vector<float>& getHeightField() const { return curr; }

private:
    int width, height;
    std::vector<float> prev, curr, next;
};

// -------------------------------
// 主程序
// -------------------------------
int main()
{
    // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Shallow Water Wave (CPU)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 800);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // 创建水波模拟器
    WaterSimulator sim(GRID_WIDTH, GRID_HEIGHT);

    // 渲染缓冲（归一化到 [0,1]）
    std::vector<float> render_buffer(GRID_WIDTH * GRID_HEIGHT);

    while (!glfwWindowShouldClose(window))
    {
        // 模拟一步
        sim.step();

        // 准备渲染数据
        const auto& h = sim.getHeightField();
        for (size_t i = 0; i < render_buffer.size(); ++i) {
            // 将高度映射到 [0,1]：假设波动范围 [-1, 1]
            render_buffer[i] = std::clamp(h[i] * 0.5f + 0.5f, 0.0f, 1.0f);
        }

        // 渲染
        glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glRasterPos2f(-1.0f, -1.0f);
        glDrawPixels(GRID_WIDTH, GRID_HEIGHT, GL_LUMINANCE, GL_FLOAT, render_buffer.data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}