
#include "stdafx.h"
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include "maths_funcs.h"

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

//mesh loader
// model structure
typedef struct Model
{
	vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	vector< vec3 > temp_vertices;
	vector< vec2 > temp_uvs;
	vector< vec3 > temp_normals;
	vector< vec3 > out_vertices;
	vector< vec2 > out_uvs;
	vector< vec3 > out_normals;
	float vertex_count;
};

Model mesh;

unsigned int teapot_vao = 0;
int width = 1024;
int height = 960;

GLuint loc1;
GLuint loc2;

//left
GLfloat rotatez = 0.0f; //rotation of child teapot around self(left)

						//parent teapot answers to keyboard event
mat4 local1T = identity_mat4();
mat4 local1R = identity_mat4();
mat4 local1S = identity_mat4();

bool key_x_pressed = false;
bool key_z_pressed = false;
bool key_s_pressed = false;
bool key_a_pressed = false;
bool key_w_pressed = false;
bool key_q_pressed = false;

mat4 T = identity_mat4(); //right, camera fly through translation

						  // Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

						  // Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "../Shaders/PhongVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Shaders/PhongFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS


bool loadMesh() {
	FILE * file = fopen("../Meshs/monkey.obj", "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}
	while (1) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop
				   //else
				   //deal with vertcies first
		if (strcmp(lineHeader, "v") == 0) {
			vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.v[0], &vertex.v[1], &vertex.v[2]);
			mesh.temp_vertices.push_back(vertex);
		}//then coordi
		else if (strcmp(lineHeader, "vt") == 0) {
			vec2 uv;
			fscanf(file, "%f %f\n", &uv.v[0], &uv.v[1]);
			mesh.temp_uvs.push_back(uv);
		}//then normal
		else if (strcmp(lineHeader, "vn") == 0) {
			vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.v[0], &normal.v[1], &normal.v[2]);
			mesh.temp_normals.push_back(normal);
		}//then faces
		else if (strcmp(lineHeader, "f") == 0) {
			string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9 ){
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			else {
				mesh.vertexIndices.push_back(vertexIndex[0]);
				mesh.vertexIndices.push_back(vertexIndex[1]);
				mesh.vertexIndices.push_back(vertexIndex[2]);
				mesh.uvIndices.push_back(uvIndex[0]);
				mesh.uvIndices.push_back(uvIndex[1]);
				mesh.uvIndices.push_back(uvIndex[2]);
				mesh.normalIndices.push_back(normalIndex[0]);
				mesh.normalIndices.push_back(normalIndex[1]);
				mesh.normalIndices.push_back(normalIndex[2]);
			}		
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}
	//processing
	//output point_count
	mesh.vertex_count = mesh.vertexIndices.size();
	// For each vertex of each triangle

	for (unsigned int i = 0; i < mesh.vertexIndices.size(); i++) {
		// Get the indices of its attributes
		unsigned int vertexIndex = mesh.vertexIndices[i];
		unsigned int uvIndex = mesh.uvIndices[i];
		unsigned int normalIndex = mesh.normalIndices[i];

		// Get the attributes thanks to the index
		vec3 vertex = mesh.temp_vertices[vertexIndex - 1];
		vec2 uv = mesh.temp_uvs[uvIndex - 1];
		vec3 normal = mesh.temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		mesh.out_vertices.push_back(vertex);
		mesh.out_uvs.push_back(uv);
		mesh.out_normals.push_back(normal);
	}
	fclose(file);

	return true;

}

void generateObjectBufferTeapot() {

	GLuint vp_vbo = 0;

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * mesh.vertex_count * sizeof(vec3), &mesh.out_vertices[0], GL_STATIC_DRAW);

	GLuint vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * mesh.vertex_count * sizeof(vec3), &mesh.out_normals[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &teapot_vao);
	glBindVertexArray(teapot_vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}


#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	// left: orthographic view from above
	// Hierarchy of Teapots
	// Root of the Hierarchy
	mat4 view = look_at(vec3(0.0, 20.0, 0.0), vec3(0.0, 0.0, -1.0), vec3(0.0, -1.0, 0.0));
	mat4 persp_proj = orthographic(120.0, (float)width / (float)height, 1, 100.0);
	//local matrix
	//scale
	mat4 local1 = scale(identity_mat4(), vec3(0.5, 0.5, 0.5));
	//answer keyboard event: scale
	//must before any translate
	//otherwise the sequence of transform wrong
	if (key_w_pressed == true) {
		local1S = scale(local1S, vec3(0.99, 0.99, 0.99));
	}
	if (key_q_pressed == true) {
		local1S = scale(local1S, vec3(1.01, 1.01, 1.01));
	}
	local1 = local1S*local1;

	//rotation
	//code below would make monkey face you
	//local1 = rotate_x_deg(local1, -90.0);
	//local1 = rotate_y_deg(local1, 180.0);
	//answer keyboard event now out of same reason
	if (key_s_pressed == true) {
		local1R = rotate_y_deg(local1R, 0.05);
	}
	if (key_a_pressed == true) {
		local1R = rotate_y_deg(local1R, -0.05);
	}
	local1 = local1R*local1;

	//translation
	//a little away from light (here along y axis) get better lit
	local1 = translate(local1, vec3(0.0, -20.0, -2.0));
	//answer keyboard event: translation
	if (key_x_pressed == true) {
		//local1T is global var, won't reset every time when display() called
		local1T = translate(local1T, vec3(-0.001f, 0.0, 0.0));
	}
	if (key_z_pressed == true) {
		local1T = translate(local1T, vec3(0.001f, 0.0, 0.0));
	}
	local1 = local1T*local1;

	// gloabal is the model matrix
	// for the root, we orient it in global space
	mat4 global1 = local1;

	// update uniforms & draw
	glViewport(0, height / 4, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global1.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count);

	// child of hierarchy
	mat4 local2 = identity_mat4();
	local2 = scale(local2, vec3(0.6, 0.6, 0.6));
	// rotate around a point: first translate, then rotate (keep rotating a unit more every time updated)
	local2 = translate(local2, vec3(0.0, 0.0, 2.0));
	local2 = rotate_y_deg(local2, rotatez);
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global2 = global1*local2;

	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count);

	// right: perspective, camera moving towards teapot and through teapot
	// moving camera
	// T is global var, which not got reset every time display() called
	T = translate(identity_mat4(), vec3(0.0, 0.0, 0.005)) * T;//degree to translate every frame
	view = translate(identity_mat4(), vec3(0.0, 0.0, -60.0));
	view = T*view;
	persp_proj = perspective(45.0, (float)width / (float)height, 0.1, 100.0);
	mat4 local0 = identity_mat4();
	mat4 global0 = local0;

	// update uniforms & draw
	glViewport(width / 2, height / 4, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global0.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count);

	glutSwapBuffers();
}



void updateScene() {

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// 0.2f as unit, rotate every frame
	rotatez += 0.01f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot();

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	if (key == 'x') {
		//Translate the base, etc.
		key_x_pressed = true;
		display();
	}

	if (key == 'z') {
		//Translate the base, etc.
		key_z_pressed = true;
		display();
	}

	if (key == 's') {
		//Rotate the base, etc.
		key_s_pressed = true;
		display();
	}

	if (key == 'a') {
		//Rotate the base, etc.
		key_a_pressed = true;
		display();
	}

	if (key == 'w') {
		//Scale the base, etc.
		key_w_pressed = true;
		display();
	}

	if (key == 'q') {
		//Scale the base, etc.
		key_q_pressed = true;
		display();
	}

}

void keyUp(unsigned char key, int x, int y) {
	if (key == 'x') {
		key_x_pressed = false;
		display();
	}

	if (key == 'z') {
		//Translate the base, etc.
		key_z_pressed = false;
		display();
	}

	if (key == 's') {
		//Rotate the base, etc.
		key_s_pressed = false;
		display();
	}

	if (key == 'a') {
		//Rotate the base, etc.
		key_a_pressed = false;
		display();
	}

	if (key == 'w') {
		//Scale the base, etc.
		key_w_pressed = false;
		display();
	}

	if (key == 'q') {
		//Scale the base, etc.
		key_q_pressed = false;
		display();
	}
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Assignment1");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	// glut keyboard callbacks
	glutKeyboardFunc(keypress);
	glutKeyboardUpFunc(keyUp);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	//load mesh : (green from lighting)monkey

	bool meshLoader = loadMesh();
	if (meshLoader == false)
	{
		fprintf(stderr, "Error: inable to load mesh\n");
		return 1;
	}
	// Set up your objects and shaders
	init();

	// Begin infinite event loop
	glutMainLoop();
	return 0;
}











