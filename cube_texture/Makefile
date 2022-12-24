all: gl30a gl30b es20 es32

gl30a: demo_cube_texture.c
	gcc demo_cube_texture.c ../glad/gl3.0a/src/glad.c -o demo_cube_texture_gl30a.exe -I../glad/gl3.0a/include `sdl2-config --libs --cflags` -ldl -Dgl30a
	
gl30b: demo_cube_texture.c
	gcc demo_cube_texture.c ../glad/gl3.0b/src/gl.c -o demo_cube_texture_gl30b.exe -I../glad/gl3.0b/include `sdl2-config --libs --cflags` -ldl -Dgl30b 
	
es20: demo_cube_texture.c
	gcc demo_cube_texture.c ../glad/gles2.0/src/glad.c -o demo_cube_texture_es20 -I../glad/gles2.0/include `sdl2-config --libs --cflags` -ldl -lm -Des20

es32: demo_cube_texture.c
	gcc demo_cube_texture.c ../glad/gles3.2/src/glad.c -o demo_cube_texture_es32 -I../glad/gles3.2/include `sdl2-config --libs --cflags` -ldl -lm -Des32
	
