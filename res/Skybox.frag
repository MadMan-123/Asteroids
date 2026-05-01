#version 420 core
out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube skybox;
// CoreShaderData UBO (binding = 0)
// CoreShaderData UBO (binding = 0)
layout(std140, binding = 0) uniform CoreShaderData {
    vec3 camPos;
    float time;
    mat4 viewMat;
    mat4 proj;
};
void main()
{
    FragColor = texture(skybox, TexCoord);
}
