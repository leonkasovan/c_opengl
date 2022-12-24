// Source: https://arm-software.github.io/opengl-es-sdk-for-android/tutorials_2_simple_cube_2jni_2_native_8cpp_source.html
// version gl3.0a, gles2.0, gles3.2: https://glad.dav1d.de/
// version gl3.0b: https://gen.glad.sh/

// gcc demo_basic.c 3.0a/src/glad.c -o demo_basic_gl30.exe -I3.0a/include `sdl2-config --libs --cflags` -ldl -Dgl30a
// gcc demo_basic.c 3.0b/src/gl.c -o demo_basic_gl30.exe -I3.0b/include `sdl2-config --libs --cflags` -ldl -Dgl30b 
// gcc demo_basic.c 2.0/src/glad.c -o demo_basic_es20 -I2.0/include `sdl2-config --libs --cflags` -ldl -Des20
// gcc demo_basic.c 3.2/src/glad.c -o demo_basic_es32 -I3.2/include `sdl2-config --libs --cflags` -ldl -Des32

#include <stdlib.h>
#include <stdio.h>

#ifdef gl30b
#include <glad/gl.h>
#else
#include <glad/glad.h>
#endif

#include <SDL.h>

const GLuint WIDTH = 600, HEIGHT = 480;
SDL_Joystick* g_joystick;
GLuint simpleCubeProgram;
GLuint vertexLocation;
GLuint vertexColourLocation;
GLuint projectionLocation;
GLuint modelViewLocation;

float projectionMatrix[16];
float modelViewMatrix[16];
float angle = 0;

GLfloat cubeVertices[] = {-1.0f,  1.0f, -1.0f, /* Back. */
						   1.0f,  1.0f, -1.0f,
						  -1.0f, -1.0f, -1.0f,
						   1.0f, -1.0f, -1.0f,
						  -1.0f,  1.0f,  1.0f, /* Front. */
						   1.0f,  1.0f,  1.0f,
						  -1.0f, -1.0f,  1.0f,
						   1.0f, -1.0f,  1.0f,
						  -1.0f,  1.0f, -1.0f, /* Left. */
						  -1.0f, -1.0f, -1.0f,
						  -1.0f, -1.0f,  1.0f,
						  -1.0f,  1.0f,  1.0f,
						   1.0f,  1.0f, -1.0f, /* Right. */
						   1.0f, -1.0f, -1.0f,
						   1.0f, -1.0f,  1.0f,
						   1.0f,  1.0f,  1.0f,
						  -1.0f, -1.0f, -1.0f, /* Top. */
						  -1.0f, -1.0f,  1.0f,
						   1.0f, -1.0f,  1.0f,
						   1.0f, -1.0f, -1.0f,
						  -1.0f,  1.0f, -1.0f, /* Bottom. */
						  -1.0f,  1.0f,  1.0f,
						   1.0f,  1.0f,  1.0f,
						   1.0f,  1.0f, -1.0f
						 };

GLfloat colour[] = {1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f
				   };
				   
GLushort indices[] = {0, 2, 3, 0, 1, 3, 4, 6, 7, 4, 5, 7, 8, 9, 10, 11, 8, 10, 12, 13, 14, 15, 12, 14, 16, 17, 18, 16, 19, 18, 20, 21, 22, 20, 23, 22};


#ifdef es32
static const char glVertexShader[] =
		"attribute vec4 vPosition;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vPosition;\n"
		"}\n";

static const char glFragmentShader[] =
		"precision mediump float;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"}\n";
#else
static const char  glVertexShader[] =
		"#version 330\n"
		"attribute vec4 vertexPosition;\n"
		"attribute vec3 vertexColour;\n"
		"varying vec3 fragColour;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 modelView;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = projection * modelView * vertexPosition;\n"
		"	fragColour = vertexColour;\n"
		"}\n";

static const char  glFragmentShader[] =
		"#version 330\n"
		"precision mediump float;\n"
		"varying vec3 fragColour;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = vec4(fragColour, 1.0);\n"
		"}\n";
#endif

void matrixIdentityFunction(float * matrix) {
	if (matrix == NULL) {
		return;
	}
	matrix[0] = 1.0f;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;
	matrix[4] = 0.0f;
	matrix[5] = 1.0f;
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 0.0f;
	matrix[9] = 0.0f;
	matrix[10] = 1.0f;
	matrix[11] = 0.0f;
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	matrix[15] = 1.0f;
}

void matrixMultiply(float * destination, float * operand1, float * operand2) {
	float theResult[16];
	int row, column = 0;
	int i, j = 0;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			theResult[4 * i + j] = operand1[j] * operand2[4 * i] + operand1[4 + j] * operand2[4 * i + 1] +
				operand1[8 + j] * operand2[4 * i + 2] + operand1[12 + j] * operand2[4 * i + 3];
		}
	}
	for (int i = 0; i < 16; i++) {
		destination[i] = theResult[i];
	}
}

void matrixTranslate(float * matrix, float x, float y, float z) {
	float temporaryMatrix[16];
	matrixIdentityFunction(temporaryMatrix);
	temporaryMatrix[12] = x;
	temporaryMatrix[13] = y;
	temporaryMatrix[14] = z;
	matrixMultiply(matrix, temporaryMatrix, matrix);
}

void matrixScale(float * matrix, float x, float y, float z) {
	float tempMatrix[16];
	matrixIdentityFunction(tempMatrix);
	tempMatrix[0] = x;
	tempMatrix[5] = y;
	tempMatrix[10] = z;
	matrixMultiply(matrix, tempMatrix, matrix);
}

float matrixDegreesToRadians(float degrees) {
	return M_PI * degrees / 180.0f;
}

void matrixRotateX(float * matrix, float angle) {
	float tempMatrix[16];
	matrixIdentityFunction(tempMatrix);
	tempMatrix[5] = cos(matrixDegreesToRadians(angle));
	tempMatrix[9] = -sin(matrixDegreesToRadians(angle));
	tempMatrix[6] = sin(matrixDegreesToRadians(angle));
	tempMatrix[10] = cos(matrixDegreesToRadians(angle));
	matrixMultiply(matrix, tempMatrix, matrix);
}

void matrixRotateY(float * matrix, float angle) {
	float tempMatrix[16];
	matrixIdentityFunction(tempMatrix);
	tempMatrix[0] = cos(matrixDegreesToRadians(angle));
	tempMatrix[8] = sin(matrixDegreesToRadians(angle));
	tempMatrix[2] = -sin(matrixDegreesToRadians(angle));
	tempMatrix[10] = cos(matrixDegreesToRadians(angle));
	matrixMultiply(matrix, tempMatrix, matrix);
}

void matrixRotateZ(float * matrix, float angle) {
	float tempMatrix[16];
	matrixIdentityFunction(tempMatrix);
	tempMatrix[0] = cos(matrixDegreesToRadians(angle));
	tempMatrix[4] = -sin(matrixDegreesToRadians(angle));
	tempMatrix[1] = sin(matrixDegreesToRadians(angle));
	tempMatrix[5] = cos(matrixDegreesToRadians(angle));
	matrixMultiply(matrix, tempMatrix, matrix);
}

void matrixFrustum(float * matrix, float left, float right, float bottom, float top, float zNear, float zFar) {
	float temp, xDistance, yDistance, zDistance;
	temp = 2.0 * zNear;
	xDistance = right - left;
	yDistance = top - bottom;
	zDistance = zFar - zNear;
	matrixIdentityFunction(matrix);
	matrix[0] = temp / xDistance;
	matrix[5] = temp / yDistance;
	matrix[8] = (right + left) / xDistance;
	matrix[9] = (top + bottom) / yDistance;
	matrix[10] = (-zFar - zNear) / zDistance;
	matrix[11] = -1.0f;
	matrix[14] = (-temp * zFar) / zDistance;
	matrix[15] = 0.0f;
}

void matrixPerspective(float * matrix, float fieldOfView, float aspectRatio, float zNear, float zFar) {
	float ymax, xmax;
	ymax = zNear * tanf(fieldOfView * M_PI / 360.0);
	xmax = ymax * aspectRatio;
	matrixFrustum(matrix, -xmax, xmax, -ymax, ymax, zNear, zFar);
}

GLuint loadShader(GLenum shaderType, const char * shaderSource) {
	GLuint shader = glCreateShader(shaderType);
	if (shader) {
		glShaderSource(shader, 1, & shaderSource, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, & compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, & infoLen);
			if (infoLen) {
				char * buf = (char * ) malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					printf("Could not Compile Shader %d:\n%s\n", shaderType, buf);
					free(buf);
				}
				glDeleteShader(shader);
				shader = 0;
			}
		}
	}
	return shader;
}

GLuint createProgram(const char * vertexSource, const char * fragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
	if (!vertexShader) {
		return 0;
	}
	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (!fragmentShader) {
		return 0;
	}
	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, & linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, & bufLength);
			if (bufLength) {
				char * buf = (char * ) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					printf("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}

void renderFrame() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	matrixIdentityFunction(modelViewMatrix);
	matrixRotateX(modelViewMatrix, angle);
	matrixRotateY(modelViewMatrix, angle);
	matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -10.0f);
	glUseProgram(simpleCubeProgram);
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, cubeVertices);
	glEnableVertexAttribArray(vertexLocation);
	glVertexAttribPointer(vertexColourLocation, 3, GL_FLOAT, GL_FALSE, 0, colour);
	glEnableVertexAttribArray(vertexColourLocation);
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	angle += 1;
	if (angle > 360) {
		angle -= 360;
	}
}

int setupGraphics(int w, int h) {
	simpleCubeProgram = createProgram(glVertexShader, glFragmentShader);
	if (simpleCubeProgram == 0) {
		printf("Could not create program\n");
		return -1;
	}
	vertexLocation = glGetAttribLocation(simpleCubeProgram, "vertexPosition");
	vertexColourLocation = glGetAttribLocation(simpleCubeProgram, "vertexColour");
	projectionLocation = glGetUniformLocation(simpleCubeProgram, "projection");
	modelViewLocation = glGetUniformLocation(simpleCubeProgram, "modelView");
	/* Setup the perspective */
	matrixPerspective(projectionMatrix, 45, (float) w / (float) h, 0.1f, 100);
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, w, h);
	return 0;
}

int main(int argc, char ** argv) {
	printf("SDL_Init\n");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
		printf(SDL_GetError());
		return -1;
	}
	if (SDL_NumJoysticks() >= 1) {
		g_joystick = SDL_JoystickOpen(0);
		if (g_joystick == NULL) {
			printf("SDL_JoystickOpen fail\n");
			return -1;
		}
		printf("SDL_JoystickOpen OK\n");
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef es20
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#ifdef es32
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
#if defined(gl30a) || defined(gl30b)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	SDL_Window * window = SDL_CreateWindow(
		"[glad] GL with SDL",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WIDTH, HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		SDL_DestroyWindow(window);
		SDL_Quit();

		fprintf(stderr, "failed to create OpenGL context: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

#if defined(es20) || defined(es32)
    int version = gladLoadGLES2Loader((GLADloadproc) SDL_GL_GetProcAddress);
#endif
#ifdef gl30a
	gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);
#endif
#ifdef gl30b
	int version = gladLoaderLoadGL();
	printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#endif
	printf("OpenGL Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
	printf("OpenGL Version:  %s\n", glGetString(GL_VERSION));
	printf("GLSL Version:	%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	setupGraphics(WIDTH, HEIGHT);
	int exit = 0;
	while (!exit) {
		SDL_Event event;
		while (SDL_PollEvent( & event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = 1;
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					exit = 1;
				}
				break;
			case SDL_JOYBUTTONUP:
				if (event.jbutton.button == 9) {
					exit = 1;
				} else {
					printf("Joy button: %d\n", event.jbutton.button);
				}
				break;
			default:
				break;
			}
		}

		//glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);
		renderFrame();

		SDL_GL_SwapWindow(window);
		SDL_Delay(5);
	}

	if (g_joystick != NULL) {
		SDL_JoystickClose(g_joystick);
		g_joystick = NULL;
	}
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
