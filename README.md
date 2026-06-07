# Game SDL2 Codebase Documentation

## Overview

This repository now has one intended rendering path:

- SDL2 creates the window and OpenGL context
- GLAD loads OpenGL functions
- GLM handles matrices and vector math
- `ObjRenderer` is the main rendering API
- `phy1` updates simulation bodies and writes transforms back into the renderer
- `grid` draws a simple XZ reference grid

The older cube and sphere rendering experiments were removed so the codebase is centered on one renderer abstraction.

## Runtime Flow

[src/main.cpp](/home/plop/projects/game_sdl2/src/main.cpp) drives the app in this order:

1. Initialize SDL video.
2. Configure an OpenGL 4.0 core context.
3. Create the window and GL context.
4. Load GL functions through GLAD.
5. Enable multisampling and depth testing.
6. Build `view` and `projection` matrices.
7. Initialize the grid renderer with `creategrid()`.
8. Construct `ObjRenderer` and call `renderer.init(...)`.
9. Add render instances with `renderer.addModelInstance(...)`.
10. Add matching physics bodies with `addDynamicBody(...)`.
11. In the frame loop:
    - process input
    - step physics with a fixed timestep
    - rebuild dirty render matrices
    - upload instance data
    - draw the objects and grid

## Using `ObjRenderer`

The public renderer API lives in [headers/obj.h](/home/plop/projects/game_sdl2/headers/obj.h) and is implemented in [src/objrender.cpp](/home/plop/projects/game_sdl2/src/objrender.cpp).

### Initialization

Call `init(...)` only after the OpenGL context exists and GLAD is loaded.

```cpp
ObjRenderer renderer;
if (!renderer.init(
    {"assets/check.obj"},
    {"assets/map/8k_sun.png"})) {
  return -1;
}
```

The first vector is the OBJ model list. The second vector is the texture list used to build a single `GL_TEXTURE_2D_ARRAY`.

### Adding instances

```cpp
int object_id = renderer.addModelInstance(
    0,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    0
);
```

Arguments:

- `model_id`
- initial position
- initial scale
- texture layer index

### Editing transforms

You can update instance transforms through:

- `renderer.translateInstance(model_id, object_id, delta)`
- `renderer.scaleInstance(model_id, object_id, delta)`
- `renderer.rotateInstance(model_id, object_id, delta_radians)`
- `renderer.transformInstance(model_id, object_id, TransformParams{...})`

These update CPU-side transform state and mark the instance dirty.

### Rebuilding and uploading

Before drawing:

```cpp
renderer.updateInstanceMatrices();
renderer.uploadInstanceData();
```

`updateInstanceMatrices()` rebuilds model matrices for dirty instances. `uploadInstanceData()` pushes matrices and integer texture-layer indices to the GPU.

### Drawing

```cpp
renderer.draw(view, projection);
```

The renderer expects the application to supply fully built GLM matrices every frame.

### Minimal frame pattern

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

renderer.updateInstanceMatrices();
renderer.uploadInstanceData();

renderer.draw(view, projection);
griddraw(view, projection);

SDL_GL_SwapWindow(window);
```

## File Documentation

### Build And Project Files

#### [Makefile](/home/plop/projects/game_sdl2/Makefile)

Build script for the project.

It:

- compiles all `src/*.cpp`
- compiles `src/glad.c`
- writes object files to `build/`
- links SDL2, SDL2_image, SDL2_ttf, and OpenGL platform libraries

Useful targets:

- `make`
- `make run`
- `make clean`

#### [CODEBASE_DOCUMENTATION.md](/home/plop/projects/game_sdl2/CODEBASE_DOCUMENTATION.md)

Repository-level documentation for the active architecture and renderer API.

#### [test.txt](/home/plop/projects/game_sdl2/test.txt)

Captured runtime/debug output from an earlier run.

### Headers

#### [headers/obj.h](/home/plop/projects/game_sdl2/headers/obj.h)

Public header for the main rendering API.

Key types:

- `ObjRenderer`
  - owns shader state, mesh assets, instance buffers, and the texture array
- `RenderInstance`
  - CPU-side transform state for one rendered object
- `TransformParams`
  - optional batch transform update structure

Key methods:

- `init(...)`
- `shutdown()`
- `addModelInstance(...)`
- `draw(...)`
- `updateInstanceMatrices()`
- `uploadInstanceData()`
- `translateInstance(...)`
- `scaleInstance(...)`
- `rotateInstance(...)`
- `transformInstance(...)`
- `getInstance(...)`
- `objectCount(...)`
- `modelCount()`

This is the main header new rendering features should depend on.

#### [headers/grid.h](/home/plop/projects/game_sdl2/headers/grid.h)

Public header for the grid helper.

Functions:

- `creategrid()`
- `griddraw(view, pers)`

#### [headers/phy.h](/home/plop/projects/game_sdl2/headers/phy.h)

Public header for the physics layer.

Key type:

- `body`
  - mass
  - position
  - velocity
  - acceleration
  - back-reference to a renderer instance via `modelID` and `objectID`

Key functions:

- `addDynamicBody(...)`
- `updateGravity()`
- `updateVelocityfirst(renderer, dt)`
- `updateVelocitysecond(renderer, dt)`
- `getphydata(id)`
- `calculateTotalEnergy(...)`

The physics layer is coupled to `ObjRenderer` because it writes transforms back into renderer instances.

#### [headers/glslread.h](/home/plop/projects/game_sdl2/headers/glslread.h)

Declares the helper used to load shader source files.

#### [headers/createWindow.h](/home/plop/projects/game_sdl2/headers/createWindow.h)

Declares a small SDL window helper that is currently not used by `main.cpp`.

### Source Files

#### [src/main.cpp](/home/plop/projects/game_sdl2/src/main.cpp)

Application entry point and orchestration layer.

Responsibilities:

- create the window and GL context
- initialize GLAD
- configure the camera
- initialize grid and object rendering
- spawn render instances and physics bodies
- process input
- advance the simulation
- update instance data and draw the frame

#### [src/objrender.cpp](/home/plop/projects/game_sdl2/src/objrender.cpp)

Implementation of `ObjRenderer`.

Responsibilities:

- own shader program and texture-array lifetime
- compile and link shaders
- load OBJ meshes through TinyObjLoader
- build VAO/VBO/EBO state per mesh
- create per-instance matrix and texture-layer buffers
- store CPU-side instance transforms
- rebuild dirty matrices
- upload instance data
- draw models with `glDrawElementsInstanced`
- clean up GPU resources in `shutdown()` and the destructor

Important internal design points:

- renderer state is owned by `ObjRenderer::RendererState`
- each loaded model has one mesh asset plus its instance arrays
- texture layers are stored as integers on the CPU and sent as integer vertex attributes
- shader/program creation is handled through dedicated helpers instead of inline repeated logic

#### [src/grid.cpp](/home/plop/projects/game_sdl2/src/grid.cpp)

Simple line renderer for the XZ reference grid.

Responsibilities:

- compile grid shaders
- generate line vertices
- upload them to a VAO/VBO
- draw the grid using the current camera matrices

#### [src/phy1.cpp](/home/plop/projects/game_sdl2/src/phy1.cpp)

Physics integration and force calculation.

Responsibilities:

- store bodies
- compute pairwise gravity
- integrate velocity and position
- write updated positions back into an `ObjRenderer` instance

Current behavior:

- brute-force `O(n^2)` gravity
- split velocity update around the gravity solve
- recentering around the center of mass in `updateVelocitysecond(...)`

#### [src/energy_calc.cpp](/home/plop/projects/game_sdl2/src/energy_calc.cpp)

Diagnostic utility for total system energy.

Useful for checking integration drift and simulation stability.

#### [src/glslread.cpp](/home/plop/projects/game_sdl2/src/glslread.cpp)

Implements the text-file helper used by shader loading.

#### [src/createWindow.cpp](/home/plop/projects/game_sdl2/src/createWindow.cpp)

Implements the simple SDL window helper declared in `headers/createWindow.h`.

#### [src/glad.c](/home/plop/projects/game_sdl2/src/glad.c)

Generated GLAD loader source. This is support code, not application logic.

### Shader Files

#### [shaders/obj_vertex.glsl](/home/plop/projects/game_sdl2/shaders/obj_vertex.glsl)

Vertex shader for `ObjRenderer`.

Inputs:

- position at location `0`
- UV at location `2`
- instance matrix at locations `3..6`
- integer texture layer at location `7`

Outputs:

- UV coordinates
- flat texture-layer index

#### [shaders/obj_fragment.glsl](/home/plop/projects/game_sdl2/shaders/obj_fragment.glsl)

Fragment shader for `ObjRenderer`.

It samples `sampler2DArray textureArray` using the per-instance texture layer.

#### [shaders/gridvex.glsl](/home/plop/projects/game_sdl2/shaders/gridvex.glsl)

Vertex shader for the grid renderer.

#### [shaders/gridfrag.glsl](/home/plop/projects/game_sdl2/shaders/gridfrag.glsl)

Fragment shader for the grid renderer.

### Notes And Assets

#### [notes/TODO.txt](/home/plop/projects/game_sdl2/notes/TODO.txt)

Short note file. The remaining items still map to the renderer work:

- fix `glBufferSubData` logic
- refactor shader creation

The shader-creation part is now partly addressed by the class-based `ObjRenderer` refactor.

#### [notes/icosphere_division.png](/home/plop/projects/game_sdl2/notes/icosphere_division.png)

Reference image kept from earlier mesh experimentation. It is not tied to active runtime code anymore.

#### Assets

The active scene currently uses:

- [assets/check.obj](/home/plop/projects/game_sdl2/assets/check.obj)
- [assets/map/8k_sun.png](/home/plop/projects/game_sdl2/assets/map/8k_sun.png)

Other OBJ and texture assets remain available for future scenes.

## Current Improvement Areas

The renderer is in much better shape now, but the next useful steps are:

1. Split mesh assets and render instances into named public/internal types instead of model buckets.
2. Add explicit material/tint support instead of texture-layer only rendering.
3. Track dirty instance ranges so uploads can use smaller `glBufferSubData(...)` regions.
4. Push more validation into the public API for failed instance creation and invalid IDs.
5. Consider moving the camera and scene orchestration out of `main.cpp` once the renderer API stabilizes.
