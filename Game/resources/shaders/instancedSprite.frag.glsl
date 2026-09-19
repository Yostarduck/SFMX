#version 330 core

in vec4 v_color;
in vec2 v_uv;

uniform sampler2D u_texture;
uniform bool      u_useTexture;

out vec4 fragColor;

void main()
{
  fragColor = u_useTexture ? v_color * texture(u_texture, v_uv) : v_color;
}
