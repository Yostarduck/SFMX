# CRT post-process shader: a custom vertex stage plus the fragment effect.
# Cooked into one multi-chunk asset (one chunk per stage) via the manifest cook hook.
# Stage files are named relative to this manifest's folder; the .glsl extension keeps
# them from being cooked as standalone assets.
vertex=crt.vert.glsl
fragment=crt.frag.glsl
