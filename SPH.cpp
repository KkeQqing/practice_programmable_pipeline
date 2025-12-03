//// main.cpp
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include <iostream>
//#include <vector>
//#include <cmath>
//#define M_PI 3.14159265358979323846
//
//// GLM for math
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//const int NUM_PARTICLES = 500;
//const float PARTICLE_MASS = 1.0f;
//const float REST_DENSITY = 1000.0f; // kg/m³
//const float GAS_STIFFNESS = 2000.0f;
//const float VISCOSITY = 250.0f;
//const float KERNEL_RADIUS = 0.04f; // h
//const float GRAVITY = 9.8f;
//const float DT = 0.001f;
//const float BOUNDARY = 0.5f; // [-BOUNDARY, BOUNDARY]^2
//
//struct Vec2 {
//    float x, y;
//    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
//    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
//    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
//    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
//    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
//    float length() const { return sqrtf(x * x + y * y); }
//    Vec2 normalize() const {
//        float l = length();
//        return l > 0 ? Vec2(x / l, y / l) : Vec2(0, 0);
//    }
//};
//
//struct Particle {
//    Vec2 pos, vel;
//    float density, pressure;
//};
//
//std::vector<Particle> particles(NUM_PARTICLES);
//
//// OpenGL objects
//GLuint VAO, VBO;
//GLuint shaderProgram;
//
//// --- Kernels ---
//float poly6(float r, float h) {
//    if (r >= h) return 0.0f;
//    float tmp = h * h - r * r;
//    return 315.0f / (64.0f * M_PI * powf(h, 9)) * powf(tmp, 3);
//}
//
//Vec2 spikyGradient(float r, float h, const Vec2& dir) {
//    if (r == 0 || r >= h) return Vec2(0, 0);
//    float tmp = h - r;
//    float grad = -45.0f / (M_PI * powf(h, 6)) * tmp * tmp;
//    return dir * grad;
//}
//
//// --- Physics ---
//void computeDensityPressure() {
//    for (int i = 0; i < NUM_PARTICLES; ++i) {
//        float sum = 0.0f;
//        for (int j = 0; j < NUM_PARTICLES; ++j) {
//            float r = (particles[i].pos - particles[j].pos).length();
//            sum += PARTICLE_MASS * poly6(r, KERNEL_RADIUS);
//        }
//        particles[i].density = sum;
//        particles[i].pressure = GAS_STIFFNESS * (particles[i].density - REST_DENSITY);
//    }
//}
//
//void computeForces() {
//    for (int i = 0; i < NUM_PARTICLES; ++i) {
//        Vec2 f_pressure(0, 0);
//        Vec2 f_viscosity(0, 0);
//
//        for (int j = 0; j < NUM_PARTICLES; ++j) {
//            if (i == j) continue;
//            Vec2 rij = particles[i].pos - particles[j].pos;
//            float r = rij.length();
//
//            if (r < KERNEL_RADIUS) {
//                // Pressure force
//                Vec2 grad = spikyGradient(r, KERNEL_RADIUS, rij);
//                float coef = -PARTICLE_MASS * (
//                    particles[i].pressure / (particles[i].density * particles[i].density) +
//                    particles[j].pressure / (particles[j].density * particles[j].density)
//                    );
//                f_pressure = f_pressure + grad * coef;
//
//                // Viscosity
//                Vec2 laplacian = (particles[j].vel - particles[i].vel) *
//                    (45.0f / (M_PI * powf(KERNEL_RADIUS, 6))) * (KERNEL_RADIUS - r);
//                f_viscosity = f_viscosity + laplacian * VISCOSITY * PARTICLE_MASS / particles[j].density;
//            }
//        }
//
//        // Gravity
//        Vec2 f_gravity(0, -GRAVITY * PARTICLE_MASS);
//
//        // Total acceleration
//        Vec2 acc = (f_pressure + f_viscosity + f_gravity) / particles[i].density;
//
//        // Update velocity and position (Euler)
//        particles[i].vel = particles[i].vel + acc * DT;
//        particles[i].pos = particles[i].pos + particles[i].vel * DT;
//
//        // Boundary handling (simple bounce)
//        if (particles[i].pos.x < -BOUNDARY) {
//            particles[i].pos.x = -BOUNDARY;
//            particles[i].vel.x *= -0.5f;
//        }
//        if (particles[i].pos.x > BOUNDARY) {
//            particles[i].pos.x = BOUNDARY;
//            particles[i].vel.x *= -0.5f;
//        }
//        if (particles[i].pos.y < -BOUNDARY) {
//            particles[i].pos.y = -BOUNDARY;
//            particles[i].vel.y *= -0.5f;
//        }
//        if (particles[i].pos.y > BOUNDARY) {
//            particles[i].pos.y = BOUNDARY;
//            particles[i].vel.y *= -0.5f;
//        }
//    }
//}
//
//// --- Init ---
//void initParticles() {
//    float spacing = 0.02f;
//    int count = 0;
//    for (float y = -0.4f; y <= -0.1f; y += spacing) {
//        for (float x = -0.3f; x <= 0.3f && count < NUM_PARTICLES; x += spacing) {
//            particles[count].pos = Vec2(x, y);
//            particles[count].vel = Vec2(0, 0);
//            count++;
//        }
//    }
//    for (int i = count; i < NUM_PARTICLES; ++i) {
//        particles[i].pos = Vec2(0, 0);
//        particles[i].vel = Vec2(0, 0);
//    }
//}
//
//// --- Rendering Setup ---
//void loadShaders() {
//    const char* vertexShaderSource = R"(
//#version 330 core
//layout (location = 0) in vec2 aPos;
//void main() {
//    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
//    gl_PointSize = 4.0;
//}
//)";
//
//    const char* fragmentShaderSource = R"(
//#version 330 core
//out vec4 FragColor;
//void main() {
//    FragColor = vec4(0.0, 0.5, 1.0, 1.0);
//}
//)";
//
//    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
//    glCompileShader(vertexShader);
//
//    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
//    glCompileShader(fragmentShader);
//
//    shaderProgram = glCreateProgram();
//    glAttachShader(shaderProgram, vertexShader);
//    glAttachShader(shaderProgram, fragmentShader);
//    glLinkProgram(shaderProgram);
//
//    glDeleteShader(vertexShader);
//    glDeleteShader(fragmentShader);
//}
//
//void setupVAO() {
//    glGenVertexArrays(1, &VAO);
//    glGenBuffers(1, &VBO);
//
//    glBindVertexArray(VAO);
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
//
//    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
//    glEnableVertexAttribArray(0);
//
//    glBindVertexArray(0);
//}
//
//// --- Render ---
//void render() {
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    std::vector<glm::vec2> positions;
//    positions.reserve(NUM_PARTICLES);
//    for (const auto& p : particles) {
//        positions.emplace_back(p.pos.x, p.pos.y);
//    }
//
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec2), positions.data());
//
//    glUseProgram(shaderProgram);
//    glBindVertexArray(VAO);
//    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
//    glBindVertexArray(0);
//    glUseProgram(0);
//}
//
//// --- Main ---
//int main() {
//    glfwInit();
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif
//
//    GLFWwindow* window = glfwCreateWindow(800, 800, "SPH Water Simulation", nullptr, nullptr);
//    if (!window) {
//        std::cerr << "Failed to create GLFW window\n";
//        return -1;
//    }
//    glfwMakeContextCurrent(window);
//
//    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//        std::cerr << "Failed to initialize GLAD\n";
//        return -1;
//    }
//
//    glViewport(0, 0, 800, 800);
//    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//
//    initParticles();
//    loadShaders();
//    setupVAO();
//
//    while (!glfwWindowShouldClose(window)) {
//        computeDensityPressure();
//        computeForces();
//
//        render();
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//
//    glDeleteVertexArrays(1, &VAO);
//    glDeleteBuffers(1, &VBO);
//    glDeleteProgram(shaderProgram);
//
//    glfwTerminate();
//    return 0;
//}