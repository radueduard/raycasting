#version 450 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_color;

layout(set = 0, binding = 0) uniform Transforms {
    mat4 Model;
    mat4 View;
    mat4 Projection;
};

layout(location = 0) out vec3 EntryPoint;
layout(location = 1) out vec4 ExitPointCoord;

void main()
{
    //TODO3
    // calculeaza EntryPoint, care este echivalenta cu culoarea per varf
    // calculeaza ExitPointCoord, care este echivalenta cu pozitia per varf (in NDC)
    EntryPoint = v_color;
    ExitPointCoord = Projection * View * Model * vec4(v_position, 1);

    gl_Position =  Projection * View * Model * vec4(v_position, 1);
}
