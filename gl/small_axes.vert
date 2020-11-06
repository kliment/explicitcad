#version 120

attribute vec3 vertex_position;
attribute vec3 vertex_color;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

varying vec3 frag_color;

void main() {
    gl_Position = projection_matrix * model_matrix * view_matrix * vec4(vertex_position, 1.0);
    frag_color = vertex_color; //gl_Position.xyz;
}

