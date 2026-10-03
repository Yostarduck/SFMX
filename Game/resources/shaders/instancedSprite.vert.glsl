#version 330 core

layout(location = 0) in vec2 a_center;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_rotBlend;   // (rotation radians, unused)

uniform mat4 u_mvp;
uniform vec2 u_sizeBegin;                   // unused: size travels per instance
uniform vec2 u_sizeEnd;

// One normalised UV rect per atlas frame: xy = origin, zw = size. Length must
// match kMaxAtlasFrames on the C++ side.
uniform vec4 u_frames[128];

// Per-instance payload: id = frame index, (x, y) = quad size in world units.
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

out vec4 v_color;
out vec2 v_uv;

void main()
{
  ParticleCustom custom = u_customData[gl_InstanceID];

  // Corner in 0..1 from the vertex id, then centred to -0.5..0.5 and scaled by
  // the per-instance size.
  vec2 corner = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
  vec2 size   = vec2(custom.x, custom.y);
  vec2 local  = (corner - 0.5) * size;

  float c = cos(a_rotBlend.x);
  float s = sin(a_rotBlend.x);
  vec2  world = vec2(local.x * c - local.y * s,
                     local.x * s + local.y * c) + a_center;

  gl_Position = u_mvp * vec4(world, 0.0, 1.0);
  v_color     = a_color;

  // Remap the 0..1 corner into this instance's atlas frame.
  vec4 frame = u_frames[custom.id];
  v_uv = frame.xy + corner * frame.zw;
}
