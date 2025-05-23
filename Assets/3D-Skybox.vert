#version 150
in vec3 vPos;

out vec3 fTexCoords;

// Built-in uniforms
uniform mat4 gObjectToWorldMatrix;
uniform mat4 gWorldToProjMatrix;

void main()
{
    fTexCoords = vPos;
    gl_Position = gWorldToProjMatrix * gObjectToWorldMatrix * vec4(vPos, 1.0f);
}
