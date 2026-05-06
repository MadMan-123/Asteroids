#version 420 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoord;

// CoreShaderData UBO (binding = 0)
layout(std140, binding = 0) uniform CoreShaderData
{
    vec3 camPos;
    float time;
    mat4 viewMat;
    mat4 proj;
};

void main()
{
    TexCoord = aPos;
    // Remove translation from view for skybox
    mat4 viewNoTrans = viewMat;
    viewNoTrans[3].xyz = vec3(0.0);
    vec4 pos = proj * viewNoTrans * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // z = w for max depth
}
