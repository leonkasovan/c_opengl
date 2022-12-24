#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int main() {
  // Open a connection to the DRM device
  int drm_fd;
  drm_fd = open("/dev/dri/card0", O_RDWR);

  // Create a GBM device
  struct gbm_device *gbm;
  gbm = gbm_create_device(drm_fd);

  // Initialize EGL
  EGLDisplay display;
  display = eglGetDisplay((EGLNativeDisplayType)gbm);
  eglInitialize(display, NULL, NULL);

  // Choose an EGL configuration
  EGLint attribList[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
  };
  EGLConfig config;
  EGLint numConfigs;
  eglChooseConfig(display, attribList, &config, 1, &numConfigs);

  // Create an EGL context
  EGLContext context;
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);

  // Get the current mode of the display
  drmModeModeInfo mode;
  drmModeCrtcPtr crtc = drmModeGetCrtc(drm_fd, crtc_id);
  mode = crtc->mode;
  drmModeFreeCrtc(crtc);

  // Create a GBM surface for the display
  struct gbm_surface *surface;
  surface = gbm_surface_create(gbm, mode.hdisplay, mode.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  // Create an EGL surface for the GBM surface
  EGLSurface egl_surface;
  egl_surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)surface, NULL);

  // Bind the EGL context to the surface
  eglMakeCurrent(display, egl_surface, egl_surface, context);

  // Set up the projection matrix
  EGLint width, height;
  eglQuerySurface(display, egl_surface, EGL_WIDTH, &width);
  eglQuerySurface(display, egl_surface, EGL_HEIGHT, &height);
  glViewport(0, 0, width, height);

  // Set up the vertex data for a triangle
  float vertices[] = {
    0.0f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f
  };

  // Set up the vertex buffer object (VBO)
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Set up the vertex array object (VAO)
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Set up the shader program
  const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
  const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  // Draw the triangle
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  // Present the rendered image to the screen
  eglSwapBuffers(display, egl_surface);

  // Clean up
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram

  // Present the rendered image to the screen
  eglSwapBuffers(display, egl_surface);

  // Clean up
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);
  eglDestroySurface(display, egl_surface);
  gbm_surface_destroy(surface);
  eglDestroyContext(display, context);
  eglTerminate(display);
  gbm_device_destroy(gbm);
  close(drm_fd);

  return 0;
}
