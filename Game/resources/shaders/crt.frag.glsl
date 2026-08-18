// Fragment stage of the CRT post-process. Fed by PostProcessPipeline: u_texture is the
// rendered scene, u_resolution the target size in pixels, u_time seconds since start.
// Deliberately strong so the effect reads at a glance: tube curvature (rounded black
// corners), RGB split, scrolling scanlines, an aperture mask and a vignette.
// Named .glsl (not .frag) so it is only pulled in by the manifest, never cooked as a
// standalone asset that would collide with crt.shader's output.
uniform sampler2D u_texture;
uniform vec2      u_resolution;
uniform float     u_time;

// Bulge the UVs outward from the centre to fake the curve of a CRT tube.
vec2 curve(vec2 uv)
{
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(5.0, 4.0);
    uv += uv * offset * offset;
    return uv * 0.5 + 0.5;
}

void main()
{
    vec2 uv = curve(gl_TexCoord[0].xy);

    // Anything outside the curved tube is the black bezel.
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Chromatic aberration: split the channels by a couple of pixels horizontally.
    float ca = 2.0 / u_resolution.x;
    vec3  color;
    color.r = texture2D(u_texture, uv + vec2(ca, 0.0)).r;
    color.g = texture2D(u_texture, uv).g;
    color.b = texture2D(u_texture, uv - vec2(ca, 0.0)).b;

    // Scanlines that scroll slowly upward.
    float scan = sin((uv.y * u_resolution.y * 0.5 - u_time * 8.0) * 3.14159);
    color *= 0.85 + 0.15 * scan * scan;

    // RGB aperture-grille mask on every third column.
    float col  = mod(gl_FragCoord.x, 3.0);
    vec3  mask = (col < 1.0) ? vec3(1.05, 0.97, 0.97)
               : (col < 2.0) ? vec3(0.97, 1.05, 0.97)
                             : vec3(0.97, 0.97, 1.05);
    color *= mask;

    // Vignette darkening toward the edges.
    vec2  c   = uv - 0.5;
    float vig = smoothstep(0.9, 0.2, dot(c, c) * 2.0);
    color *= mix(0.5, 1.2, vig);

    // Faint mains flicker.
    color *= 0.98 + 0.02 * sin(u_time * 10.0);

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
