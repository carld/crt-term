#version 150

in vec3 Color;
in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D tex;
uniform float appTime;
uniform vec2 sourceSize;
uniform vec2 targetSize;

void main()
{
    outColor = texture2D(tex, Texcoord.xy);
}

