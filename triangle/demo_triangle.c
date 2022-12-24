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
GLuint simpleTriangleProgram;
GLuint vPosition;

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
static const char glVertexShader[] =
	"#version 330\n"
        "attribute vec4 vPosition;\n"
        "void main()\n"
        "{\n"
        "  gl_Position = vPosition;\n"
        "}\n";

static const char glFragmentShader[] =
	"#version 330\n"
        "precision mediump float;\n"
        "void main()\n"
        "{\n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n";
#endif

GLuint loadShader(GLenum shaderType, const char* shaderSource)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader)
    {
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen)
            {
                char * buf = (char*) malloc(infoLen);
                if (buf)
                {
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

GLuint createProgram(const char* vertexSource, const char * fragmentSource)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader)
    {
        return 0;
    }
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader)
    {
        return 0;
    }
    GLuint program = glCreateProgram();
    if (program)
    {
        glAttachShader(program , vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program , GL_LINK_STATUS, &linkStatus);
        if( linkStatus != GL_TRUE)
        {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength)
            {
                char* buf = (char*) malloc(bufLength);
                if (buf)
                {
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

const GLfloat triangleVertices[] = {
        0.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f
};

void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(simpleTriangleProgram);
    glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0 ,triangleVertices);
    glEnableVertexAttribArray(vPosition);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int setupGraphics(int w, int h)
{
    simpleTriangleProgram = createProgram(glVertexShader, glFragmentShader);
    if (!simpleTriangleProgram)
    {
        printf("Could not create program\n");
        return -1;
    }
    vPosition = glGetAttribLocation(simpleTriangleProgram, "vPosition");
    glViewport(0, 0, w, h);
    return 0;
}


int main(int argc, char **argv) {
	printf("SDL_Init\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0){
		printf(SDL_GetError());
		return -1;
	}
	if (SDL_NumJoysticks() >= 1){
		g_joystick = SDL_JoystickOpen(0);
		if (g_joystick == NULL){
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
    

    SDL_Window *window = SDL_CreateWindow(
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
    printf("GLSL Version:    %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	setupGraphics(WIDTH, HEIGHT);
    int exit = 0;
    while(!exit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
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
                    }else{
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
        SDL_Delay(1);
    }

   if (g_joystick != NULL)
   {
      SDL_JoystickClose(g_joystick);
      g_joystick = NULL;
   }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

