#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "gfx.h"

//The Windows computer I was developing on had an Intel graphics card with terrible support for openGL.
//Comment out the following 'define' if you have a Windows computer (Linux users are fine here, just get mesa) with "good enough" openGL support.
//All I know for sure is openGL 2.1 isn't enough, but openGL 3.2 probably is.
#ifdef WINDOWS
#define STUPIDINTEL
#endif

static GLuint uniColorId, vbo;

int width2, height2;

void gettinRealSickOfThis(){
	GLenum error = glGetError();
	if(error){
		fputs((const char*)gluErrorString(error), stderr);
		fputc('\n', stderr);
		fflush(stderr);
	}
}

void initGfx(){
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	const GLchar* vertexPrg =
"#version 120\n"
"attribute vec2 pos;"
//"in vec3 color;"
//"out vec3 fragColor;"
"void main(){"
//"fragColor = color;"
"gl_Position = vec4(pos, 0.0, 1.0);"
"}";
	const GLchar* fragmentPrg =
"#version 120\n"
//"in vec3 fragColor;"
"uniform vec3 uniColor;"
//"out vec4 outColor;"
"void main(){"
"gl_FragColor = vec4(uniColor, 1.0);"
"}";
	GLuint vertexPrgId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexPrgId, 1, &vertexPrg, NULL);
	glCompileShader(vertexPrgId);
	GLuint fragmentPrgId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentPrgId, 1, &fragmentPrg, NULL);
	glCompileShader(fragmentPrgId);
	GLuint prgId = glCreateProgram();
	glAttachShader(prgId, vertexPrgId);
	glAttachShader(prgId, fragmentPrgId);
//	glBindFragDataLocation(prgId, 0, "outColor");
	glLinkProgram(prgId);
	glUseProgram(prgId);

	GLint status;
	glGetShaderiv(vertexPrgId, GL_COMPILE_STATUS, &status);
	if(status!=GL_TRUE){
		fputs("OMG AAAAAAAAAAAAAAA\n", stderr);
		char buffer[512];
		glGetShaderInfoLog(vertexPrgId, 512, NULL, buffer);
		fputs(buffer, stderr);
		fputc('\n', stderr);
	}
	glGetShaderiv(fragmentPrgId, GL_COMPILE_STATUS, &status);
	if(status!=GL_TRUE){
		fputs("OMG AAAAAAAAHHHHHHH\n", stderr);
		char buffer[512];
		glGetShaderInfoLog(fragmentPrgId, 512, NULL, buffer);
		fputs(buffer, stderr);
		fputc('\n', stderr);
	}

	GLint posAttrib = glGetAttribLocation(prgId, "pos");
//	GLint colorAttrib = glGetAttribLocation(prgId, "color");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
//	glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(2*sizeof(float)));
	glEnableVertexAttribArray(posAttrib);
//	glEnableVertexAttribArray(colorAttrib);
	uniColorId = glGetUniformLocation(prgId, "uniColor");
}

void setColorFromHex(uint32_t color){
	glUniform3f(uniColorId, (float)(color&0xFF000000)/0xFF000000, (float)(color&0xFF0000)/0xFF0000, (float)(color&0xFF00)/0xFF00);
}

void setColor(float r, float g, float b){
	glUniform3f(uniColorId, r, g, b);
}

void drawLine(float x1, float y1, float x2, float y2){
	float points[]={x1, y1, x2, y2};
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);
	glDrawArrays(GL_LINES, 0, 2);
#ifndef STUPIDINTEL
	glMapBufferRange(GL_ARRAY_BUFFER, 0, 0, GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT|GL_MAP_INVALIDATE_BUFFER_BIT);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	//glInvalidateBufferData(vbo);
#endif
}
