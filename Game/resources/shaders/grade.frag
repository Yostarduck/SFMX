// Full-screen post-process: warm colour grade + vignette.
// Fed by PostProcessPipeline: u_texture is the rendered scene, u_resolution the
// target size in pixels, u_time seconds since start (for the gentle vignette pulse).
uniform sampler2D u_texture;
uniform vec2      u_resolution;
uniform float     u_time;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 color = texture2D(u_texture, uv);

    // Colour grade: slight lift + a warm tint.
    color.rgb = pow(color.rgb, vec3(0.95));
    color.rgb *= vec3(1.06, 1.0, 0.94);

    // Vignette that breathes very slightly over time.
    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float edge = 0.78 + 0.02 * sin(u_time * 0.8);
    float vignette = smoothstep(edge, 0.35, dist);
    color.rgb *= mix(0.55, 1.0, vignette);

    gl_FragColor = color;
}
