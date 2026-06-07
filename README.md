# Game SDL2

SDL2 + OpenGL project with a class-based OBJ renderer and a simple gravity simulation.

For file-by-file codebase notes, see [other_info.md](/home/plop/projects/game_sdl2/other_info.md).

## What This Project Does

The current application:

- creates an SDL2 window with an OpenGL 4.0 core context
- loads OpenGL through GLAD
- renders OBJ models through `ObjRenderer`
- draws a reference grid
- runs a simple N-body gravity simulation
- lets you move a free camera around the scene

The main rendering API is `ObjRenderer` in [headers/obj.h](/home/plop/projects/game_sdl2/headers/obj.h).

## Build And Run

Build:

```bash
make
```

Run:

```bash
make run
```

Clean:

```bash
make clean
```

## Runtime Controls

- `W` / `S`: move forward and backward
- `A` / `D`: strafe left and right
- `Space`: move up
- `Left Shift`: move down
- `Mouse`: look around
- window close: quit

## Renderer Usage

This section is the intended usage pattern for `ObjRenderer`.

### 1. Create the OpenGL context first

Do not initialize the renderer before SDL and GLAD are ready.

Required order:

1. `SDL_Init(SDL_INIT_VIDEO)`
2. `SDL_GL_SetAttribute(...)`
3. `SDL_CreateWindow(..., SDL_WINDOW_OPENGL)`
4. `SDL_GL_CreateContext(window)`
5. `gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)`

After that, you can safely construct and initialize `ObjRenderer`.

### 2. Initialize the renderer

```cpp
ObjRenderer renderer;

if (!renderer.init(
    {"assets/check.obj"},
    {"assets/map/8k_sun.png"})) {
  return -1;
}
```

Meaning:

- the first vector is the list of OBJ models
- the second vector is the list of textures used to build one `GL_TEXTURE_2D_ARRAY`

Index rules:

- OBJ path index becomes `model_id`
- texture path index becomes `texture_layer`

If you pass two OBJ files, model `0` is the first file and model `1` is the second. The same rule applies to textures.

### 3. Add model instances

Use `addModelInstance(...)` to spawn visible objects in the scene.

```cpp
int object_id = renderer.addModelInstance(
    0,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    0
);
```

Parameters:

- `model_id`: which loaded OBJ mesh to instance
- `position`: world position
- `scale`: object scale
- `texture_layer`: which texture layer from the texture array to sample

Return value:

- non-negative value: valid `object_id`
- `-1`: invalid `model_id`

### 4. Upload instance data before the first draw

After creating instances, push them to the GPU:

```cpp
renderer.uploadInstanceData();
```

If you forget this, your objects may exist on the CPU side but not appear correctly on screen.

### 5. Build camera matrices

`ObjRenderer` does not create camera matrices for you. It expects:

- a `view` matrix
- a `projection` matrix

Typical setup:

```cpp
glm::vec3 camera_pos(0.0f, 0.0f, -20.0f);
glm::vec3 target_dir(0.0f, 0.0f, 0.0f);
glm::vec3 world_up(0.0f, 1.0f, 0.0f);

glm::mat4 view = glm::lookAt(camera_pos, target_dir, world_up);
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),
    aspect_ratio,
    0.01f,
    1000.0f
);
```

### 6. Draw every frame

Per-frame usage:

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

renderer.updateInstanceMatrices();
renderer.uploadInstanceData();
renderer.draw(view, projection);

griddraw(view, projection);
SDL_GL_SwapWindow(window);
```

What each step does:

- `updateInstanceMatrices()`: rebuilds model matrices for dirty objects
- `uploadInstanceData()`: uploads those matrices and texture-layer indices
- `draw(view, projection)`: draws all model instances

## Transforming Objects

There are two ways to change an object transform.

### Incremental updates

Use these when you want to move or rotate relative to the current state:

```cpp
renderer.translateInstance(0, object_id, glm::vec3{1.0f, 0.0f, 0.0f});
renderer.scaleInstance(0, object_id, glm::vec3{0.2f, 0.2f, 0.2f});
renderer.rotateInstance(0, object_id, glm::vec3{0.0f, 0.5f, 0.0f});
```

These modify the stored transform and mark the object dirty.

### Absolute updates

Use `TransformParams` when you want to replace one or more transform fields directly:

```cpp
TransformParams params;
params.position = glm::vec3{5.0f, 2.0f, 0.0f};
params.scale = glm::vec3{2.0f, 2.0f, 2.0f};
params.rotation = glm::vec3{0.0f, 1.0f, 0.0f};

renderer.transformInstance(0, object_id, params);
```

You only need to set the fields you want to change.

Example:

```cpp
TransformParams params;
params.position = glm::vec3{0.0f, 10.0f, 0.0f};
renderer.transformInstance(0, object_id, params);
```

### Reading back instance data

```cpp
RenderInstance &instance = renderer.getInstance(0, object_id);
glm::vec3 current_position = instance.position;
```

Use this when another system, such as physics, needs the current render transform.

## Working With Multiple Models And Textures

You can load more than one model and more than one texture:

```cpp
ObjRenderer renderer;
renderer.init(
    {"assets/check.obj", "assets/earth.obj"},
    {"assets/map/8k_sun.png", "assets/map/8k_earth_daymap.jpg"}
);
```

Then instantiate them by index:

```cpp
int sun = renderer.addModelInstance(
    0,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{2.0f, 2.0f, 2.0f},
    0
);

int earth = renderer.addModelInstance(
    1,
    glm::vec3{10.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    1
);
```

This means:

- `sun` uses model `assets/check.obj` with texture layer `0`
- `earth` uses model `assets/earth.obj` with texture layer `1`

## Using The Physics Layer

The physics system is tied to renderer instance IDs. The normal pattern is:

1. create a render instance
2. create a physics body that points to that instance
3. step physics every frame
4. let physics write positions back into the renderer

Example:

```cpp
int object_id = renderer.addModelInstance(
    0,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    0
);

renderer.uploadInstanceData();

size_t body_id = addDynamicBody(
    0,
    object_id,
    100.0,
    glm::vec3(renderer.getInstance(0, object_id).position)
);

getphydata(body_id).velocity = glm::dvec3{0.0, 1.0, 0.0};
```

In the fixed-step update:

```cpp
while (accumulator >= dt) {
  updateVelocityfirst(renderer, dt);
  updateGravity();
  updateVelocitysecond(renderer, dt);
  accumulator -= dt;
}
```

After physics changes transforms, the normal render path still applies:

```cpp
renderer.updateInstanceMatrices();
renderer.uploadInstanceData();
renderer.draw(view, projection);
```

## Minimal Example

This is the smallest useful renderer flow in one place:

```cpp
SDL_Init(SDL_INIT_VIDEO);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

SDL_Window* window = SDL_CreateWindow(
    "OpenGL Test",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    1280,
    720,
    SDL_WINDOW_OPENGL
);

SDL_GL_CreateContext(window);
gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

glEnable(GL_DEPTH_TEST);

ObjRenderer renderer;
if (!renderer.init({"assets/check.obj"}, {"assets/map/8k_sun.png"})) {
  return -1;
}

int object_id = renderer.addModelInstance(
    0,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    0
);

renderer.uploadInstanceData();

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, -20.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),
    1280.0f / 720.0f,
    0.01f,
    1000.0f
);

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
renderer.updateInstanceMatrices();
renderer.uploadInstanceData();
renderer.draw(view, projection);
SDL_GL_SwapWindow(window);
```

## Common Mistakes

- Initializing `ObjRenderer` before the OpenGL context exists.
- Forgetting to call `renderer.uploadInstanceData()` after adding instances.
- Changing transforms and forgetting the `updateInstanceMatrices()` + `uploadInstanceData()` step before drawing.
- Passing an invalid `model_id` to `addModelInstance(...)`.
- Assuming textures are separate OpenGL textures per object. They are layers in one texture array.
- Mixing up model index and object instance index.

## Where To Look Next

- Usage and integration example: [src/main.cpp](/home/plop/projects/game_sdl2/src/main.cpp)
- Public renderer API: [headers/obj.h](/home/plop/projects/game_sdl2/headers/obj.h)
- Renderer implementation: [src/objrender.cpp](/home/plop/projects/game_sdl2/src/objrender.cpp)
- File-by-file documentation: [other_info.md](/home/plop/projects/game_sdl2/other_info.md)
