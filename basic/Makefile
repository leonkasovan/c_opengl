all: gl30a gl30b es20 es32

gl30a: demo_basic.c
	gcc demo_basic.c ../glad/gl3.0a/src/glad.c -o demo_basic_gl30a.exe -I../glad/gl3.0a/include `sdl2-config --libs --cflags` -ldl -Dgl30a
	
gl30b: demo_basic.c
	gcc demo_basic.c ../glad/gl3.0b/src/gl.c -o demo_basic_gl30b.exe -I../glad/gl3.0b/include `sdl2-config --libs --cflags` -ldl -Dgl30b 
	
es20: demo_basic.c
	gcc demo_basic.c ../glad/gles2.0/src/glad.c -o demo_basic_es20 -I../glad/gles2.0/include `sdl2-config --libs --cflags` -ldl -Des20

es32: demo_basic.c
	gcc demo_basic.c ../glad/gles3.2/src/glad.c -o demo_basic_es32 -I../glad/gles3.2/include `sdl2-config --libs --cflags` -ldl -Des32
	

