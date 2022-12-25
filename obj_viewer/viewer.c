#include <memory.h>
#include <stdbool.h>
#include <string.h>
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <float.h>
#include <limits.h>
#include <math.h>
#include "tinyobj_loader_c.h"
#include <glad/glad.h>
#ifdef _WIN64
#define atoll(S) _atoi64(S)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <SDL.h>
#include "trackball.h"

#define DEAD_ZONE 9999

void gluLookAt(GLdouble eyeX,GLdouble eyeY,GLdouble eyeZ,GLdouble centerX,GLdouble centerY,GLdouble centerZ,GLdouble upX,GLdouble upY,GLdouble upZ);
void gluPerspective(GLdouble fovy,GLdouble aspect,GLdouble zNear,GLdouble zFar);

typedef struct {
	GLuint vb;
	int numTriangles;
}
DrawObject;

static DrawObject gDrawObject;

static int WIDTH = 640;
static int HEIGHT = 480;

static bool use_colors = false;
static bool draw_wireframe = true;

static
const size_t OBJ_SIZE = sizeof(float) * 3 // pos
	+
	sizeof(float) * 3 // normal
	+
	sizeof(float) * 3 // color (based on normal)
	+
	sizeof(float) * 3; // color from material file.

static float prevMouseX, prevMouseY;
static int mouseLeftPressed;
static int mouseMiddlePressed;
static int mouseRightPressed;
static float curr_quat[4];
static float prev_quat[4];
static float eye[3], lookat[3], up[3];

static void CheckErrors(const char * desc) {
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc, e, e);
		exit(20);
	}
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
	float v10[3];
	float v20[3];
	float len2;

	v10[0] = v1[0] - v0[0];
	v10[1] = v1[1] - v0[1];
	v10[2] = v1[2] - v0[2];

	v20[0] = v2[0] - v0[0];
	v20[1] = v2[1] - v0[1];
	v20[2] = v2[2] - v0[2];

	N[0] = v20[1] * v10[2] - v20[2] * v10[1];
	N[1] = v20[2] * v10[0] - v20[0] * v10[2];
	N[2] = v20[0] * v10[1] - v20[1] * v10[0];

	len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
	if (len2 > 0.0f) {
		float len = (float) sqrt((double) len2);

		N[0] /= len;
		N[1] /= len;
	}
}

static char * mmap_file(size_t * len,
	const char * filename) {
#ifdef _WIN64
	HANDLE file =
		CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		/* E.g. Model may not have materials. */
		return NULL;
	}

	HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	assert(fileMapping != INVALID_HANDLE_VALUE);

	LPVOID fileMapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
	char * fileMapViewChar = (char * ) fileMapView;
	assert(fileMapView != NULL);

	DWORD file_size = GetFileSize(file, NULL);
	( * len) = (size_t) file_size;

	return fileMapViewChar;
#else

	struct stat sb;
	char * p;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return NULL;
	}

	if (fstat(fd, & sb) == -1) {
		perror("fstat");
		return NULL;
	}

	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s is not a file\n", filename);
		return NULL;
	}

	p = (char * ) mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

	if (p == MAP_FAILED) {
		perror("mmap");
		return NULL;
	}

	if (close(fd) == -1) {
		perror("close");
		return NULL;
	}

	( * len) = sb.st_size;

	return p;

#endif
}

/* path will be modified */
static char * get_dirname(char * path) {
	char * last_delim = NULL;

	if (path == NULL) {
		return path;
	}

#if defined(_WIN32) || defined(_WIN64)
	/* TODO: Unix style path */
	last_delim = strrchr(path, '\\');
#else
	last_delim = strrchr(path, '/');
#endif

	if (last_delim == NULL) {
		/* no delimiter in the string. */
		return path;
	}

	/* remove '/' */
	last_delim[0] = '\0';

	return path;
}

static void get_file_data(void * ctx,
	const char * filename,
		const int is_mtl,
			const char * obj_filename, char ** data, size_t * len) {
	// NOTE: If you allocate the buffer with malloc(),
	// You can define your own memory management struct and pass it through `ctx`
	// to store the pointer and free memories at clean up stage(when you quit an
	// app)
	// This example uses mmap(), so no free() required.
	(void) ctx;

	if (!filename) {
		fprintf(stderr, "null filename\n");
		( * data) = NULL;
		( * len) = 0;
		return;
	}

	size_t data_len = 0;

	* data = mmap_file( & data_len, filename);
	( * len) = data_len;
}

static int LoadObjAndConvert(float bmin[3], float bmax[3],
	const char * filename) {
	tinyobj_attrib_t attrib;
	tinyobj_shape_t * shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t * materials = NULL;
	size_t num_materials;

	{
		unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
		int ret =
			tinyobj_parse_obj( & attrib, & shapes, & num_shapes, & materials, &
				num_materials, filename, get_file_data, NULL, flags);
		if (ret != TINYOBJ_SUCCESS) {
			return 0;
		}

		printf("# of shapes	= %d\n", (int) num_shapes);
		printf("# of materials = %d\n", (int) num_materials);

		/*
		{
		int i;
		for (i = 0; i < num_shapes; i++) {
		printf("shape[%d] name = %s\n", i, shapes[i].name);
		}
		}
		*/
	}

	bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
	bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;

	{
		DrawObject o;
		float * vb;
		/* std::vector<float> vb; //  */
		size_t face_offset = 0;
		size_t i;

		/* Assume triangulated face. */
		size_t num_triangles = attrib.num_face_num_verts;
		size_t stride =
			OBJ_SIZE /
			sizeof(float);

		vb = (float * ) malloc(OBJ_SIZE * num_triangles * 3);

		for (i = 0; i < attrib.num_face_num_verts; i++) {
			size_t f;
			assert(attrib.face_num_verts[i] % 3 ==
				0); /* assume all triangle faces. */
			for (f = 0; f < (size_t) attrib.face_num_verts[i] / 3; f++) {
				size_t k;
				float v[3][3];
				float n[3][3];
				float c[3];
				float len2;

				tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
				tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
				tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];

				for (k = 0; k < 3; k++) {
					int f0 = idx0.v_idx;
					int f1 = idx1.v_idx;
					int f2 = idx2.v_idx;
					assert(f0 >= 0);
					assert(f1 >= 0);
					assert(f2 >= 0);

					v[0][k] = attrib.vertices[3 * (size_t) f0 + k];
					v[1][k] = attrib.vertices[3 * (size_t) f1 + k];
					v[2][k] = attrib.vertices[3 * (size_t) f2 + k];
					bmin[k] = (v[0][k] < bmin[k]) ? v[0][k] : bmin[k];
					bmin[k] = (v[1][k] < bmin[k]) ? v[1][k] : bmin[k];
					bmin[k] = (v[2][k] < bmin[k]) ? v[2][k] : bmin[k];
					bmax[k] = (v[0][k] > bmax[k]) ? v[0][k] : bmax[k];
					bmax[k] = (v[1][k] > bmax[k]) ? v[1][k] : bmax[k];
					bmax[k] = (v[2][k] > bmax[k]) ? v[2][k] : bmax[k];
				}

				if (attrib.num_normals > 0) {
					int f0 = idx0.vn_idx;
					int f1 = idx1.vn_idx;
					int f2 = idx2.vn_idx;
					if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
						assert(f0 < (int) attrib.num_normals);
						assert(f1 < (int) attrib.num_normals);
						assert(f2 < (int) attrib.num_normals);
						for (k = 0; k < 3; k++) {
							n[0][k] = attrib.normals[3 * (size_t) f0 + k];
							n[1][k] = attrib.normals[3 * (size_t) f1 + k];
							n[2][k] = attrib.normals[3 * (size_t) f2 + k];
						}
					} else {
						/* normal index is not defined for this face */
						/* compute geometric normal */
						CalcNormal(n[0], v[0], v[1], v[2]);
						n[1][0] = n[0][0];
						n[1][1] = n[0][1];
						n[1][2] = n[0][2];
						n[2][0] = n[0][0];
						n[2][1] = n[0][1];
						n[2][2] = n[0][2];
					}
				} else {
					/* compute geometric normal */
					CalcNormal(n[0], v[0], v[1], v[2]);
					n[1][0] = n[0][0];
					n[1][1] = n[0][1];
					n[1][2] = n[0][2];
					n[2][0] = n[0][0];
					n[2][1] = n[0][1];
					n[2][2] = n[0][2];
				}

				for (k = 0; k < 3; k++) {
					vb[(3 * i + k) * stride + 0] = v[k][0];
					vb[(3 * i + k) * stride + 1] = v[k][1];
					vb[(3 * i + k) * stride + 2] = v[k][2];
					vb[(3 * i + k) * stride + 3] = n[k][0];
					vb[(3 * i + k) * stride + 4] = n[k][1];
					vb[(3 * i + k) * stride + 5] = n[k][2];

					/* Set the normal as alternate color */
					c[0] = n[k][0];
					c[1] = n[k][1];
					c[2] = n[k][2];
					len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
					if (len2 > 0.0f) {
						float len = (float) sqrt((double) len2);

						c[0] /= len;
						c[1] /= len;
						c[2] /= len;
					}

					vb[(3 * i + k) * stride + 6] = (c[0] * 0.5f + 0.5f);
					vb[(3 * i + k) * stride + 7] = (c[1] * 0.5f + 0.5f);
					vb[(3 * i + k) * stride + 8] = (c[2] * 0.5f + 0.5f);

					/* now set the color from the material */
					if (attrib.material_ids[i] >= 0) {
						int matidx = attrib.material_ids[i];
						vb[(3 * i + k) * stride + 9] = materials[matidx].diffuse[0];
						vb[(3 * i + k) * stride + 10] = materials[matidx].diffuse[1];
						vb[(3 * i + k) * stride + 11] = materials[matidx].diffuse[2];
					} else {
						/* Just copy the default value */
						vb[(3 * i + k) * stride + 9] = vb[(3 * i + k) * stride + 6];
						vb[(3 * i + k) * stride + 10] = vb[(3 * i + k) * stride + 7];
						vb[(3 * i + k) * stride + 11] = vb[(3 * i + k) * stride + 8];
					}

				}
			}
			/* You can access per-face material through attrib.material_ids[i] */

			face_offset += (size_t) attrib.face_num_verts[i];
		}

		o.vb = 0;
		o.numTriangles = 0;
		if (num_triangles > 0) {
			glGenBuffers(1, & o.vb);
			glBindBuffer(GL_ARRAY_BUFFER, o.vb);
			glBufferData(GL_ARRAY_BUFFER,
				(GLsizeiptr)(num_triangles * 3 * stride * sizeof(float)), vb,
				GL_STATIC_DRAW);
			o.numTriangles = (int) num_triangles;
		}

		free(vb);

		gDrawObject = o;
	}

	printf("bmin = %f, %f, %f\n", (double) bmin[0], (double) bmin[1],
		(double) bmin[2]);
	printf("bmax = %f, %f, %f\n", (double) bmax[0], (double) bmax[1],
		(double) bmax[2]);

	tinyobj_attrib_free( & attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);

	return 1;
}

static void Draw(const DrawObject * draw_object) {
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.0);
	glColor3f(1.0f, 1.0f, 1.0f);
	if (draw_object -> vb >= 1) {
		glBindBuffer(GL_ARRAY_BUFFER, draw_object -> vb);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, OBJ_SIZE, (const void * ) 0);
		glNormalPointer(GL_FLOAT, OBJ_SIZE, (const void * )(sizeof(float) * 3));
		if (use_colors) {
			glColorPointer(3, GL_FLOAT, OBJ_SIZE, (const void * )(sizeof(float) * 9));
		} else {
			glColorPointer(3, GL_FLOAT, OBJ_SIZE, (const void * )(sizeof(float) * 6));
		}
		glDrawArrays(GL_TRIANGLES, 0, 3 * draw_object -> numTriangles);
		CheckErrors("drawarrays");
	}

	/* draw wireframe */
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	glColor3f(0.0f, 0.0f, 0.4f);

	if (draw_object -> vb >= 1 && draw_wireframe) {
		glBindBuffer(GL_ARRAY_BUFFER, draw_object -> vb);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, OBJ_SIZE, (const void * ) 0);
		glNormalPointer(GL_FLOAT, OBJ_SIZE, (const void * )(sizeof(float) * 3));

		glDrawArrays(GL_TRIANGLES, 0, 3 * draw_object -> numTriangles);
		CheckErrors("drawarrays");
	}
}

SDL_Window *window;
SDL_GLContext context;
SDL_Joystick *g_joystick;

int SetupSystem() {
	printf("SDL_Init\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0){
		puts(SDL_GetError());
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
#if defined(_WIN32) || defined(_WIN64)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
    

    window = SDL_CreateWindow(
        "[glad] GL with SDL",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    context = SDL_GL_CreateContext(window);
	if (context == NULL) {
        SDL_DestroyWindow(window);
        SDL_Quit();

        fprintf(stderr, "failed to create OpenGL context: %s\n", SDL_GetError());
        return -1;
    }

#if defined(_WIN32) || defined(_WIN64)
	gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);
#else
	gladLoadGLES2Loader((GLADloadproc) SDL_GL_GetProcAddress);
#endif
    printf("OpenGL Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version:  %s\n", glGetString(GL_VERSION));
    printf("GLSL Version:    %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	trackball(curr_quat, 0, 0, 0, 0);

	eye[0] = 0.0f;
	eye[1] = 0.0f;
	eye[2] = 3.0f;

	lookat[0] = 0.0f;
	lookat[1] = 0.0f;
	lookat[2] = 0.0f;

	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;
	
	glViewport(0, 0, WIDTH, HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLdouble) WIDTH / (GLdouble) HEIGHT, (GLdouble) 0.01f,(GLdouble) 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	return 0;
}

int CleanUpSystem(){
	if (g_joystick != NULL)
   {
      SDL_JoystickClose(g_joystick);
      g_joystick = NULL;
   }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char ** argv) {
	float bmin[3], bmax[3];
	float maxExtent;

#if defined(WIN32)
	printf("WIN32\n");
#endif
#if defined(_WIN32)
	printf("_WIN32\n");
#endif
#if defined(_WIN64)
	printf("_WIN64\n");
#endif
#if defined(__linux__)
	printf("__linux__\n");
#endif
#if defined(_POSIX_VERSION)
	printf("_POSIX_VERSION\n");
#endif

	if (argc < 2) {
		printf("Needs input.obj\n");
		return -1;
	}
	
	if (SetupSystem()){
		printf("Setup System fail\n");
		return -1;
	}

	if (0 == LoadObjAndConvert(bmin, bmax, argv[1])) {
		printf("failed to load & conv\n");
		return -1;
	}

	maxExtent = 0.5f * (bmax[0] - bmin[0]);
	if (maxExtent < 0.5f * (bmax[1] - bmin[1])) {
		maxExtent = 0.5f * (bmax[1] - bmin[1]);
	}
	if (maxExtent < 0.5f * (bmax[2] - bmin[2])) {
		maxExtent = 0.5f * (bmax[2] - bmin[2]);
	}

	int exit = 0;
	while (!exit) {
		GLfloat mat[4][4];
		
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
					if (event.jbutton.button == 7 || event.jbutton.button == 6) {
                        exit = 1;
                    }else if (event.jbutton.button == 4){
						eye[2] += 2.0f * ((float) 200 - 280) / (float) HEIGHT;
						lookat[2] += 2.0f * ((float) 200 - 280) / (float) HEIGHT;
					}else if (event.jbutton.button == 5){
						eye[2] += 2.0f * ((float) 280 - 200) / (float) HEIGHT;
						lookat[2] += 2.0f * ((float) 280 - 200) / (float) HEIGHT;
					}
					printf("Joy button: %d\n", event.jbutton.button);
                    break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						mouseLeftPressed = 1;
						trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
					}else if (event.button.button == SDL_BUTTON_RIGHT){
						mouseRightPressed = 1;
					}else if (event.button.button == SDL_BUTTON_MIDDLE){
						mouseMiddlePressed = 1;
					}
					break;
                case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT) {
						mouseLeftPressed = 0;
					}else if (event.button.button == SDL_BUTTON_RIGHT){
						mouseRightPressed = 0;
					}else if (event.button.button == SDL_BUTTON_MIDDLE){
						mouseMiddlePressed = 0;
					}
					break;
				case SDL_MOUSEMOTION:
					float rotScale = 1.0f;
					float transScale = 2.0f;

					if (mouseLeftPressed) {
						trackball(prev_quat, rotScale * (2.0f * prevMouseX - WIDTH) / (float) WIDTH,
							rotScale * (HEIGHT - 2.0f * prevMouseY) / (float) HEIGHT,
							rotScale * (2.0f * (float) event.motion.x - WIDTH) / (float) WIDTH,
							rotScale * (HEIGHT - 2.0f * (float) event.motion.y) / (float) HEIGHT);
						add_quats(prev_quat, curr_quat, curr_quat);
					} else if (mouseMiddlePressed) {
						eye[0] -= transScale * ((float) event.motion.x - prevMouseX) / (float) WIDTH;
						lookat[0] -= transScale * ((float) event.motion.x - prevMouseX) / (float) WIDTH;
						eye[1] += transScale * ((float) event.motion.y - prevMouseY) / (float) HEIGHT;
						lookat[1] += transScale * ((float) event.motion.y - prevMouseY) / (float) HEIGHT;
					} else if (mouseRightPressed) {
						eye[2] += transScale * ((float) event.motion.y - prevMouseY) / (float) HEIGHT;
						lookat[2] += transScale * ((float) event.motion.y - prevMouseY) / (float) HEIGHT;
					}

					prevMouseX = (float) event.motion.x;
					prevMouseY = (float) event.motion.y;
				case SDL_JOYAXISMOTION:
					if (event.jaxis.axis == 0){
						if (event.jaxis.value < -DEAD_ZONE){
							eye[0] -= transScale * ((float) -20) / (float) WIDTH;
							lookat[0] -= transScale * ((float) -20) / (float) WIDTH;
						}else if (event.jaxis.value > DEAD_ZONE){
							eye[0] -= transScale * ((float) 20) / (float) WIDTH;
							lookat[0] -= transScale * ((float) 20) / (float) WIDTH;
						}
					}else if (event.jaxis.axis == 1){
						if (event.jaxis.value < -DEAD_ZONE){
							eye[1] += transScale * ((float) -10) / (float) HEIGHT;
							lookat[1] += transScale * ((float) -10) / (float) HEIGHT;
						}else if (event.jaxis.value > DEAD_ZONE){
							eye[1] += transScale * ((float) 10) / (float) HEIGHT;
							lookat[1] += transScale * ((float) 10) / (float) HEIGHT;
						}
					}
					if (event.jaxis.axis == 2){
						if (event.jaxis.value < -DEAD_ZONE){
							trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
							trackball(prev_quat, rotScale * (2.0f * 300 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * 240) / (float) HEIGHT,
								rotScale * (2.0f * (float) 340 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * (float) 240) / (float) HEIGHT);
							add_quats(prev_quat, curr_quat, curr_quat);
						}else if (event.jaxis.value > DEAD_ZONE){
							trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
							trackball(prev_quat, rotScale * (2.0f * 340 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * 240) / (float) HEIGHT,
								rotScale * (2.0f * (float) 300 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * (float) 240) / (float) HEIGHT);
							add_quats(prev_quat, curr_quat, curr_quat);
						}
					}else if (event.jaxis.axis == 3){
						if (event.jaxis.value < -DEAD_ZONE){
							trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
							trackball(prev_quat, rotScale * (2.0f * 320 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * 280) / (float) HEIGHT,
								rotScale * (2.0f * (float) 320 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * (float) 200) / (float) HEIGHT);
							add_quats(prev_quat, curr_quat, curr_quat);
						}else if (event.jaxis.value > DEAD_ZONE){
							trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
							trackball(prev_quat, rotScale * (2.0f * 320 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * 200) / (float) HEIGHT,
								rotScale * (2.0f * (float) 320 - WIDTH) / (float) WIDTH,
								rotScale * (HEIGHT - 2.0f * (float) 280) / (float) HEIGHT);
							add_quats(prev_quat, curr_quat, curr_quat);
						}
					}
					
					break;
				default:
                    break;
            }
        }
		
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt((GLdouble) eye[0], (GLdouble) eye[1], (GLdouble) eye[2],
			(GLdouble) lookat[0], (GLdouble) lookat[1], (GLdouble) lookat[2],
			(GLdouble) up[0], (GLdouble) up[1], (GLdouble) up[2]);
		build_rotmatrix(mat, curr_quat);
		glMultMatrixf( & mat[0][0]);

		/* Fit to -1, 1 */
		glScalef(1.0f / maxExtent, 1.0f / maxExtent, 1.0f / maxExtent);

		/* Centerize object. */
		glTranslatef(-0.5f * (bmax[0] + bmin[0]), -0.5f * (bmax[1] + bmin[1]),
			-0.5f * (bmax[2] + bmin[2]));

		Draw( & gDrawObject);

		SDL_GL_SwapWindow(window);
	}

	CleanUpSystem();
	return 0;
}