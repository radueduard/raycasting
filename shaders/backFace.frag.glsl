#version 450 core

layout(location = 0) in vec3 frag_color;

layout(location = 0) out vec4 out_color;

void main()
{
    //TODO2
    //calculeaza out_color in functie de frag_color
    out_color = vec4(frag_color, 1.0);
}