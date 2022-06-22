# DoonEngine
DoonEngine is a voxel path tracing engine that calculates lighting per-voxel instead of per-pixel. It allows for dynamic streaming of voxels from the CPU and fast edits to the map. It is being developed only by one person and is still a work-in-progress, so don't expect too much polish.

# Overview
The voxels are separated into 8x8x8 chunks, which allows for faster dynamic editing as only a portion of the map needs to be reuploaded when it is edited. Additionally, empty chunks are able to be skipped over when ray casting. When drawing the map, each chunk that is visible to the camera is added to a buffer. Each chunk in this buffer then has its lighting updated, allowing for per-voxel lighting. The diffuse lighting is pure path-traced and accumulates over many frames. The direct light and specular component, however, are calculated by shooting a fixed number of uniformly-spaced rays so that they can be updated instantly as the camera moves and the map is edited.

# Development Videos
https://www.youtube.com/watch?v=OAF4RCS_pPc

https://www.youtube.com/watch?v=T0hunImo9Hs
