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

        glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

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

