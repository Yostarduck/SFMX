#version 330 core

layout(location = 0) in vec2 a_center;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_rotBlend;

uniform mat4 u_mvp;
uniform vec2 u_sizeBegin;
uniform vec2 u_sizeEnd;

struct ParticleCustom
{
  int   id;
  float x;
  float y;
  float z;
};

layout(std140) uniform ParticleCustomData
{
  ParticleCustom u_customData[1024];
};

flat out int v_customId;
out vec4     v_custom;
out vec4     v_color;
out vec2     v_uv;

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

void main()
{
  ParticleCustom custom = u_customData[gl_InstanceID];
  v_customId = custom.id;
  v_custom   = vec4(custom.x, custom.y, custom.z, 0.0);

  int totalDigits = numberOfDigits();

  vec2  uv    = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
  vec2  size  = mix(u_sizeBegin, u_sizeEnd, a_rotBlend.y);
  size.x = size.x * totalDigits;
  vec2  local = (uv * 2.0 - 1.0) * size * 0.5;

  float c = cos(a_rotBlend.x);
  float s = sin(a_rotBlend.x);
  vec2  world = vec2(local.x * c - local.y * s,
                     local.x * s + local.y * c) + a_center;

  gl_Position = u_mvp * vec4(world, 0.0, 1.0);
  v_color     = a_color;
  uv.x = uv.x * float(totalDigits);
  v_uv        = uv;
}
