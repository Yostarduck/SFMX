# Particle material for the instanced quad renderer. Both stages are required:
# the vertex one feeds the generic instance attributes and reads the per-instance
# custom-data uniform block, which is the only thing that can observe it.
vertex=particleCustom.vert.glsl
fragment=particleCustom.frag.glsl
