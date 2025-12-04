//// stable_fluids.cpp
//// Based on Jos Stam's "Stable Fluids" (SIGGRAPH 1999)
//// 2D Incompressible Navier-Stokes Solver with Density (Smoke)
//
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <iostream>
//#include <vector>
//#include <cmath>
//#include <algorithm>
//
//const int N = 64;              // Grid resolution (N x N)
//const float dt = 0.016f;       // Time step
//const float visc = 0.0001f;    // Viscosity
//const float diff = 0.0001f;    // Diffusion rate for density
//const int solver_iter = 20;    // Jacobi iterations for pressure & diffusion
//
//// Fluid fields
//float u[N + 2][N + 2], v[N + 2][N + 2];        // Velocity (with 1-cell ghost boundary)
//float u_prev[N + 2][N + 2], v_prev[N + 2][N + 2];
//float dens[N + 2][N + 2], dens_prev[N + 2][N + 2];
//float p[N + 2][N + 2];                     // Pressure
//
//GLuint shaderProgram;
//GLuint quadVAO, quadVBO;
//GLuint densityTexture;
//
//// --- Helper: Set boundary conditions ---
//void set_bnd(int b, float x[N + 2][N + 2]) {
//    for (int i = 1; i <= N; i++) {
//        x[0][i] = b == 1 ? -x[1][i] : x[1][i];
//        x[N + 1][i] = b == 1 ? -x[N][i] : x[N][i];
//        x[i][0] = b == 2 ? -x[i][1] : x[i][1];
//        x[i][N + 1] = b == 2 ? -x[i][N] : x[i][N];
//    }
//    x[0][0] = 0.5f * (x[1][0] + x[0][1]);
//    x[0][N + 1] = 0.5f * (x[1][N + 1] + x[0][N]);
//    x[N + 1][0] = 0.5f * (x[N][0] + x[N + 1][1]);
//    x[N + 1][N + 1] = 0.5f * (x[N][N + 1] + x[N + 1][N]);
//}
//
//// --- Linear solver (Jacobi) for diffusion or pressure ---
//void lin_solve(int b, float x[N + 2][N + 2], float x0[N + 2][N + 2], float a, float c) {
//    for (int k = 0; k < solver_iter; k++) {
//        for (int i = 1; i <= N; i++) {
//            for (int j = 1; j <= N; j++) {
//                x[i][j] = (x0[i][j] + a * (x[i - 1][j] + x[i + 1][j] + x[i][j - 1] + x[i][j + 1])) / c;
//            }
//        }
//        set_bnd(b, x);
//    }
//}
//
//// --- Diffuse velocity or density ---
//void diffuse(int b, float x[N + 2][N + 2], float x0[N + 2][N + 2], float diff) {
//    float a = dt * diff * N * N;
//    lin_solve(b, x, x0, a, 1 + 4 * a);
//}
//
//// --- Advect using Semi-Lagrangian backtrace ---
//void advect(int b, float d[N + 2][N + 2], float d0[N + 2][N + 2], float u[N + 2][N + 2], float v[N + 2][N + 2]) {
//    float dt0 = dt * N;
//    for (int i = 1; i <= N; i++) {
//        for (int j = 1; j <= N; j++) {
//            float x = i - dt0 * u[i][j];
//            float y = j - dt0 * v[i][j];
//
//            // Clamp to [0.5, N+0.5]
//            if (x < 0.5f) x = 0.5f;
//            if (x > N + 0.5f) x = N + 0.5f;
//            if (y < 0.5f) y = 0.5f;
//            if (y > N + 0.5f) y = N + 0.5f;
//
//            int i0 = (int)x, i1 = i0 + 1;
//            int j0 = (int)y, j1 = j0 + 1;
//
//            float s1 = x - i0, s0 = 1.0f - s1;
//            float t1 = y - j0, t0 = 1.0f - t1;
//
//            d[i][j] = s0 * (t0 * d0[i0][j0] + t1 * d0[i0][j1]) +
//                s1 * (t0 * d0[i1][j0] + t1 * d0[i1][j1]);
//        }
//    }
//    set_bnd(b, d);
//}
//
//// --- Project velocity to divergence-free field ---
//void project(float u[N + 2][N + 2], float v[N + 2][N + 2], float p[N + 2][N + 2], float div[N + 2][N + 2]) {
//    // Compute divergence
//    for (int i = 1; i <= N; i++) {
//        for (int j = 1; j <= N; j++) {
//            div[i][j] = -0.5f * (u[i + 1][j] - u[i - 1][j] + v[i][j + 1] - v[i][j - 1]) / N;
//            p[i][j] = 0;
//        }
//    }
//    set_bnd(0, div); set_bnd(0, p);
//
//    // Solve Poisson equation: ∇²p = div
//    lin_solve(0, p, div, 1, 4);
//
//    // Subtract gradient of pressure
//    for (int i = 1; i <= N; i++) {
//        for (int j = 1; j <= N; j++) {
//            u[i][j] -= 0.5f * (p[i + 1][j] - p[i - 1][j]) * N;
//            v[i][j] -= 0.5f * (p[i][j + 1] - p[i][j - 1]) * N;
//        }
//    }
//    set_bnd(1, u); set_bnd(2, v);
//}
//
//// --- Main fluid step ---
//void fluid_step() {
//    // --- Velocity Step ---
//    diffuse(1, u_prev, u, visc);
//    diffuse(2, v_prev, v, visc);
//    advect(1, u, u_prev, u_prev, v_prev);
//    advect(2, v, v_prev, u_prev, v_prev);
//    project(u, v, p, u_prev); // reuse u_prev as div buffer
//
//    // --- Density Step ---
//    diffuse(0, dens_prev, dens, diff);
//    advect(0, dens, dens_prev, u, v);
//}
//
//// --- Add density and velocity at mouse position ---
//void add_source(int x, int y, float amount = 100.0f) {
//    if (x < 1 || x > N || y < 1 || y > N) return;
//    dens[x][y] += amount;
//    u[x][y] += (rand() % 1000) / 500.0f - 1.0f; // random x impulse
//    v[x][y] += 2.0f;                             // upward impulse
//}
//
//// --- OpenGL: Fullscreen quad shader ---
//const char* vertexShaderSource = R"(
//#version 330 core
//layout (location = 0) in vec2 aPos;
//layout (location = 1) in vec2 aTexCoord;
//out vec2 TexCoord;
//void main() {
//    gl_Position = vec4(aPos, 0.0, 1.0);
//    TexCoord = aTexCoord;
//}
//)";
//
//const char* fragmentShaderSource = R"(
//#version 330 core
//in vec2 TexCoord;
//out vec4 FragColor;
//uniform sampler2D densityTex;
//void main() {
//    float d = texture(densityTex, TexCoord).r;
//    // Map density to smoke color (dark gray to white)
//    vec3 color = vec3(0.1, 0.1, 0.1) + d * vec3(0.9, 0.92, 0.95);
//    FragColor = vec4(color, 1.0);
//}
//)";
//
//GLuint createShaderProgram() {
//    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vs, 1, &vertexShaderSource, NULL);
//    glCompileShader(vs);
//
//    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
//    glCompileShader(fs);
//
//    GLuint prog = glCreateProgram();
//    glAttachShader(prog, vs);
//    glAttachShader(prog, fs);
//    glLinkProgram(prog);
//
//    glDeleteShader(vs); glDeleteShader(fs);
//    return prog;
//}
//
//void initRender() {
//    // Fullscreen quad
//    float quadVertices[] = {
//        -1.0f,  1.0f, 0.0f, 1.0f,
//        -1.0f, -1.0f, 0.0f, 0.0f,
//         1.0f,  1.0f, 1.0f, 1.0f,
//         1.0f, -1.0f, 1.0f, 0.0f,
//    };
//
//    glGenVertexArrays(1, &quadVAO);
//    glGenBuffers(1, &quadVBO);
//    glBindVertexArray(quadVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
//    glBindVertexArray(0);
//
//    // Density texture
//    glGenTextures(1, &densityTexture);
//    glBindTexture(GL_TEXTURE_2D, densityTexture);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, N, N, 0, GL_RED, GL_FLOAT, NULL);
//}
//
//
//// 替换原来的 clamp 函数
//inline float clamp(float x, float a, float b) {
//    if (x < a) return a;
//    if (x > b) return b;
//    return x;
//}
//
//void render() {
//    // Upload density to GPU
//    std::vector<float> texData(N * N);
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//            texData[i * N + j] = clamp(dens[i + 1][j + 1], 0.0f, 1.0f);
//        }
//    }
//    glBindTexture(GL_TEXTURE_2D, densityTexture);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RED, GL_FLOAT, texData.data());
//
//    // Render fullscreen quad
//    glClear(GL_COLOR_BUFFER_BIT);
//    glUseProgram(shaderProgram);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, densityTexture);
//    glUniform1i(glGetUniformLocation(shaderProgram, "densityTex"), 0);
//    glBindVertexArray(quadVAO);
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//    glBindVertexArray(0);
//}
//
//// Mouse interaction
//double mouseX = -1, mouseY = -1;
//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
//    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
//        glfwGetCursorPos(window, &mouseX, &mouseY);
//    }
//    else {
//        mouseX = mouseY = -1;
//    }
//}
//
//
//
//// Main
//int main() {
//    glfwInit();
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif
//
//    GLFWwindow* window = glfwCreateWindow(512, 512, "Stable Fluids (Navier-Stokes)", nullptr, nullptr);
//    if (!window) { std::cerr << "Window failed\n"; return -1; }
//    glfwMakeContextCurrent(window);
//    glfwSetMouseButtonCallback(window, mouse_button_callback);
//
//    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//        std::cerr << "GLAD init failed\n"; return -1;
//    }
//
//    glViewport(0, 0, 512, 512);
//    shaderProgram = createShaderProgram();
//    initRender();
//
//    // Initialize fields to zero
//    for (int i = 0; i < N + 2; i++)
//        for (int j = 0; j < N + 2; j++)
//            u[i][j] = v[i][j] = dens[i][j] = p[i][j] = 0.0f;
//
//    while (!glfwWindowShouldClose(window)) {
//        // Add source at mouse
//        if (mouseX >= 0 && mouseY >= 0) {
//            int gridX = (int)(mouseX * N / 512.0);
//            int gridY = (int)((512.0 - mouseY) * N / 512.0); // invert Y
//            add_source(gridX + 1, gridY + 1); // +1 for ghost cell offset
//        }
//
//        fluid_step();
//
//        render();
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//
//    glfwTerminate();
//    return 0;
//}