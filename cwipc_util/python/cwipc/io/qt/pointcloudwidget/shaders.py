"""
Contains the shader programs used by the renderer.
"""

"""
Simple shader.
"""

VERTEX_SHADER_SIMPLE: str = """
#version 430 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 input_color;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

out vec3 color;

void main() {
    gl_Position = projection_matrix * modelview_matrix * vec4(position, 1.0);
    color = input_color;
}
"""

VERTEX_SHADER_SIZED_POINTS: str = """
#version 430 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 input_color;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform float point_size_factor;

out vec3 color;

void main() {
    vec4 position_relative_to_camera = modelview_matrix * vec4(position, 1.0);
    gl_Position = projection_matrix * position_relative_to_camera;
    gl_PointSize = point_size_factor / length(position_relative_to_camera);
    color = input_color;
}
"""

FRAGMENT_SHADER: str = """
#version 430 core

in vec3 color;
out vec4 output_color;

void main() {
    output_color = vec4(color, 1.0);
}
"""
