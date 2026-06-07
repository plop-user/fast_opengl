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

