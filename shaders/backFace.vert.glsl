#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

// Uniforme
layout (set = 0, binding = 0) uniform Transformations {
    mat4 Model;
    mat4 View;
    mat4 Projection;
};

layout(location = 0) out vec3 frag_color;

void main()
{
    //TODO1
    //calculaza frag_color pentru a fi transmis mai departe catre fragment shader
    frag_color = color;

    gl_Position = Projection * View * Model * vec4(position, 1);
}
