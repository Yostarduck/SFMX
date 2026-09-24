# Atlas material for the instanced quad renderer, used by InstanceDrawer. Both
# stages are required: the vertex one feeds the generic instance attributes,
# reads per-instance size + frame from the custom-data block, and looks the
# frame's UV rect up in u_frames.
vertex=instancedSprite.vert.glsl
fragment=instancedSprite.frag.glsl
