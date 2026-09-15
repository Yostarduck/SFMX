#version 330 core

flat in int v_customId;
in vec4     v_custom;
in vec4     v_color;
in vec2     v_uv;

uniform sampler2D u_texture;
uniform bool      u_useTexture;

out vec4 fragColor;

int numberOfDigits()
{
  int remaining = abs(v_customId);
  int digits    = 1;
  
  while (remaining >= 10) {
    remaining /= 10;
    ++digits;
  }

  return digits;
}

int getDigit(int index)
{
  int digits = numberOfDigits();
  if (index < 0 || index >= digits) {
    return 0;
  }

  int value = abs(v_customId);

  for (int i = 0; i < digits - 1 - index; ++i) {
    value /= 10;
  }

  return value % 10;
}

void main()
{
  int i = int(floor(v_uv.x));

  vec2 uv = v_uv;
  
  uv.x = (fract(uv.x) * 0.1) + (getDigit(i) * 0.1);
  
  vec4 base = u_useTexture ? v_color * texture(u_texture, uv) : v_color;

  fragColor = vec4(base.rgb, base.a);
}
