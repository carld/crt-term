#version 150

in vec3 Color;
in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D tex;

uniform vec2 sourceSize;
uniform vec2 targetSize;

int samples = 5; // pixels per axis; higher = bigger glow, worse performance
float quality = 2.0; // lower = smaller glow, better quality

void main()
{
  vec4 source = texture(tex, Texcoord);
  vec4 sum = vec4(0);
  int diff = (samples - 1) / 2;
  vec2 sizeFactor = vec2(1) / targetSize * quality;
  
  for (int x = -diff; x <= diff; x++)
  {
    for (int y = -diff; y <= diff; y++)
    {
      vec2 offset = vec2(x, y) * sizeFactor;
      sum += texture(tex, Texcoord + offset);
    }
  }
  
  outColor = ((sum / (samples * samples)) + source) * vec4(Color, 10);
}
