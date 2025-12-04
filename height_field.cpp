// water_heightfield.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// 定义水面网格分辨率：128×128 个顶点
const int GRID_WIDTH = 128;  // 网格宽度
const int GRID_HEIGHT = 128; // 网格高度
const float GRID_SIZE = 1.0f; // 每个格子的物理尺寸（米）,用于渲染和计算

// 波动参数
const float DAMPING = 0.995f; // 阻尼系数,模拟能量损失.每帧对波浪高度乘以该值，模拟水的粘滞阻力（能量衰减）。值越接近 1，波浪衰减越慢。
const float WAVE_SPEED = 2.0f; // 波速,波在水面上传播的速度（单位：米/秒）
const float TIME_STEP = 1.0f / 60.0f; // 时间步长,模拟帧率60FPS

// 水面高度场
float height[GRID_HEIGHT][GRID_WIDTH] = { 0 }; // 当前水面高度
float prev_height[GRID_HEIGHT][GRID_WIDTH] = { 0 }; // 上一帧高度

// 渲染数据
GLuint VAO, VBO, EBO; // 顶点数组对象（封装顶点属性），顶点缓冲对象（储存顶点位置），索引缓冲对象（存储三角形索引）
GLuint shaderProgram; // 着色器程序

// 摄像机
glm::vec3 cameraPos = glm::vec3(0.0f, 20.0f, 30.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
// 鼠标控制
bool firstMouse = true; // 是否首次移动鼠标
float yaw = -90.0f, pitch = -20.0f;
float lastX = 400, lastY = 300;
float movementSpeed = 5.0f; // 摄像机移动速度
float mouseSensitivity = 0.1f; // 鼠标灵敏度

// 鼠标扰动位置
int disturbX = -1, disturbY = -1; // 鼠标扰动位置，-1表示无扰动

// --- Shader Sources ---
// 顶点着色器
const char* vertexShaderSource = R"( 
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D heightMap; 
out vec3 Normal;
out vec3 FragPos;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;

    // 简单法线
    Normal = normalize(vec3(0.0, 1.0, 0.0));
    // 问题：法线恒为 (0,1,0)，未反映波浪起伏。
}
)";

// 片段着色器
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // 环境光
    vec3 ambient = 0.1 * vec3(0.0, 0.3, 0.6);

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.0, 0.5, 1.0);

    // 镜面反射（简单）
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * vec3(1.0, 1.0, 1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 0.8);
}
)";

// --- Shader Compilation ---
GLuint createShaderProgram() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// --- 初始化网格 ---
void initGrid() {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    // 生成顶点（Y=0，后续动态更新）
    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            float worldX = (x - GRID_WIDTH / 2.0f) * GRID_SIZE;
            float worldZ = (z - GRID_HEIGHT / 2.0f) * GRID_SIZE;
            vertices.push_back(glm::vec3(worldX, 0.0f, worldZ));
        }
    }

    // 生成索引（三角形）
    for (int z = 0; z < GRID_HEIGHT - 1; ++z) {
        for (int x = 0; x < GRID_WIDTH - 1; ++x) {
            unsigned int i = z * GRID_WIDTH + x;
            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + GRID_WIDTH);

            indices.push_back(i + 1);
            indices.push_back(i + GRID_WIDTH + 1);
            indices.push_back(i + GRID_WIDTH);
        }
    }

    // VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// --- 水面更新（有限差分）---
void updateWater() {
    // 边界固定为0（可改为反射等）
    for (int i = 1; i < GRID_HEIGHT - 1; ++i) {
        for (int j = 1; j < GRID_WIDTH - 1; ++j) {
            float laplacian =
                height[i - 1][j] + height[i + 1][j] +
                height[i][j - 1] + height[i][j + 1] - 4 * height[i][j];

            float c2_dt2_dx2 = (WAVE_SPEED * WAVE_SPEED * TIME_STEP * TIME_STEP) / (GRID_SIZE * GRID_SIZE);
            float newH = 2 * height[i][j] - prev_height[i][j] + c2_dt2_dx2 * laplacian;
            newH *= DAMPING;

            prev_height[i][j] = height[i][j];
            height[i][j] = newH;
        }
    }

    // 鼠标扰动
    if (disturbX >= 0 && disturbY >= 0) {
        int x = disturbX;
        int y = disturbY;
        if (x >= 1 && x < GRID_WIDTH - 1 && y >= 1 && y < GRID_HEIGHT - 1) {
            height[y][x] += 2.0f;
        }
        disturbX = disturbY = -1; // 一次扰动
    }
}

// --- 更新 VBO 高度 ---
void updateVertexBuffer() {
    std::vector<glm::vec3> tempVertices(GRID_WIDTH * GRID_HEIGHT);
    for (int z = 0; z < GRID_HEIGHT; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            float worldX = (x - GRID_WIDTH / 2.0f) * GRID_SIZE;
            float worldZ = (z - GRID_HEIGHT / 2.0f) * GRID_SIZE;
            float worldY = height[z][x] * 2.0f; // 放大波浪
            tempVertices[z * GRID_WIDTH + x] = glm::vec3(worldX, worldY, worldZ);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, tempVertices.size() * sizeof(glm::vec3), tempVertices.data());
}

// --- 摄像机控制 ---
void processInput(GLFWwindow* window, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    movementSpeed += (float)yoffset;
    if (movementSpeed < 1.0f) movementSpeed = 1.0f;
    if (movementSpeed > 20.0f) movementSpeed = 20.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // 将屏幕坐标转换为网格坐标（简化：假设正交投影中心对齐）
        float ndcX = (2.0f * xpos / 800.0f - 1.0f);
        float ndcY = 1.0f - (2.0f * ypos / 800.0f); // invert Y

        // 假设摄像机俯视，近似映射（更精确需射线求交）
        disturbX = (int)((ndcX * 30.0f + GRID_WIDTH / 2.0f));
        disturbY = (int)((ndcY * 30.0f + GRID_HEIGHT / 2.0f));

        if (disturbX < 0) disturbX = 0;
        if (disturbX >= GRID_WIDTH) disturbX = GRID_WIDTH - 1;
        if (disturbY < 0) disturbY = 0;
        if (disturbY >= GRID_HEIGHT) disturbY = GRID_HEIGHT - 1;
    }
}

// --- Main ---
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 800, "Height Field Water Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 800);

    shaderProgram = createShaderProgram();
    initGrid();

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);
        updateWater();
        updateVertexBuffer();

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(10.0f, 20.0f, 10.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GRID_WIDTH - 1) * (GRID_HEIGHT - 1) * 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}