#version 150

in vec2 position;
in vec2 texcoord;

out vec3 Color;
out vec2 Texcoord;

void main()
{
  Color = vec3(1.0,1.0,1.0);
  Texcoord = texcoord;
  gl_Position = vec4(position.xy, 0.0, 1.0);
}

