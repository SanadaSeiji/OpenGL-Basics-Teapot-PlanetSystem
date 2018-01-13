#version 330



layout (location = 0) in vec3 vertex_position;

layout (location = 1) in vec3 vertex_normals;

//light need to be able to move/change colour
uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

out vec3 position_eye, normal_eye; //will load from mesh.obj into Model mesh



void main () {

//raising everything to eye_space
//later useful in fragment shader
	position_eye = vec3 (view * model * vec4 (vertex_position, 1.0));

	normal_eye = vec3 (view * model * vec4 (vertex_normals, 0.0));

	gl_Position = proj * vec4 (position_eye, 1.0);

}