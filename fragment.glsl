#version 150

in vec3 Color;
in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D tex;
uniform float appTime;

void main()
{
    outColor = texture2D(tex, Texcoord.xy);
}

