// Vertex stage of the CRT post-process. Proves the pipeline can run a custom vertex
// shader: it gently "breathes" the fullscreen quad (a tiny overscan zoom around the
// screen centre) — a geometry-level move a fragment shader cannot do. It also has to
// reproduce SFML's default outputs so the fragment stage still gets its 0..1 UVs.
// u_time / u_resolution are shared with the fragment stage (set once per pass).
// Named .glsl (not .vert/.frag) so it is only pulled in by the manifest, never cooked
// as a standalone asset that would collide with crt.shader's output.
uniform float u_time;
uniform vec2  u_resolution;

void main()
{
    vec2  center = u_resolution * 0.5;
    float scale  = 1.0 + 0.01 * (0.5 + 0.5 * sin(u_time * 1.0));
    vec2  pos    = center + (gl_Vertex.xy - center) * scale;

    gl_Position    = gl_ModelViewProjectionMatrix * vec4(pos, gl_Vertex.z, gl_Vertex.w);
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;   // normalised sprite UVs
    gl_FrontColor  = gl_Color;
}
