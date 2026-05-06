#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

layout (std140, binding = 0) uniform CoreShaderData
{
    vec3 camPos;
    float time;
    mat4 view;
    mat4 projection;
};

// Per-entity model matrix — used for single draws
uniform mat4 model;

// Instanced model matrices (binding = 1).
// Written per-frame by rendererDefaultArchetypeRender when batching same-model entities.
// u_modelBaseIndex >= 0 activates instancing: reads u_instanceModels[base + gl_InstanceID].
layout (std430, binding = 1) buffer InstanceMatrices {
    mat4 u_instanceModels[];
};
uniform int u_modelBaseIndex = -1; // -1 = single draw using 'model' uniform (default)
uniform float u_vertexSnapGridSize = 128.0; // PSX vertex snapping grid size (32.0 = prominent PS1 jitter, 0.0 = disabled)

out vec2 tc;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;
noperspective out float affineW; // For PS1-style affine texture mapping

vec4 snapVertexGrid(vec4 clipPos, float gridSize)
{
    if (gridSize <= 0.0) return clipPos;
    
    // Perspective divide to get normalized device coordinates
    vec3 ndc = clipPos.xyz / clipPos.w;
    
    // Snap to grid in NDC space (simulates PS1's integer vertex positions)
    ndc.xy = floor(ndc.xy * gridSize) / gridSize;
    
    // Convert back to clip space
    return vec4(ndc * clipPos.w, clipPos.w);
}

void main()
{
    mat4 actualModel = (u_modelBaseIndex >= 0)
        ? u_instanceModels[u_modelBaseIndex + gl_InstanceID]
        : model;

    vec4 worldPos = actualModel * vec4(position, 1.0);
    FragPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(actualModel)));
    Normal = normalize(normalMatrix * normal);

    // Gram-Schmidt tangent frame for normal mapping
    vec3 N = Normal;
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    tc = texCoord;
    vec4 clipPos = projection * view * worldPos;
    gl_Position = snapVertexGrid(clipPos, u_vertexSnapGridSize);
    affineW = gl_Position.w;
}
