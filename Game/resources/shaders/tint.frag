// Per-object material: multiply the sprite by a tint colour.
// u_texture is the sprite's own texture (bound by MaterialComponent as the current
// texture); u_tint is set by the material (see DemoScene).
uniform sampler2D u_texture;
uniform vec4      u_tint;

void main()
{
    vec4 texColor = texture2D(u_texture, gl_TexCoord[0].xy);
    gl_FragColor = texColor * gl_Color * u_tint;
}
