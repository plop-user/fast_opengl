# Game SDL2

SDL2 + OpenGL project with an OBJ renderer, a reference grid, a skybox, and a simple gravity simulation.

For file-by-file notes, see [other_info.md](/home/plop/projects/mygame/other_info.md).

## What This Project Does

The current application:

- creates an SDL2 window with an OpenGL 4.0 core context
- loads OpenGL through GLAD
- renders OBJ models through `ObjRenderer`
- supports `.obj` + `.mtl` diffuse textures via `map_Kd`
- draws a reference grid
- runs a simple N-body gravity simulation
- lets you move a free camera around the scene

The main rendering API is `ObjRenderer` in [headers/obj.h](/home/plop/projects/mygame/headers/obj.h).

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

This section describes the current `ObjRenderer` API and behavior.

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
if (!renderer.init()) {
  return -1;
}
```

`init()` only compiles and links the object shaders. It does not load models or textures.

### 3. Load meshes

Each call to `loadMesh()` returns a `model_id`.

```cpp
int sphere = renderer.loadMesh("assets/check.obj");
int ship = renderer.loadMesh(
    "assets/4c275vutixts-PrometheusNX59650/Prometheus NX 59650/prometheus.obj");
```

Rules:

- `model_id` is the return value from `loadMesh()`
- `-1` means the load failed
- if the OBJ references an `.mtl`, the renderer loads `map_Kd` diffuse textures automatically
- `.mtl` texture paths are resolved relative to the OBJ file
- JPG and PNG diffuse textures are supported
- mixed texture sizes are supported

For a multi-material asset like `prometheus.obj`, this is enough to pull in the color textures referenced by `prometheus.mtl`.

### 4. Optional: load standalone override textures

You can still load a texture manually and use it as a whole-instance override.

```cpp
int sun_tex = renderer.loadTexture("assets/map/8k_sun.png");
```

The returned value is a texture override id that can be assigned through `InstanceMaterial::texture_layer`.

### 5. Add model instances

Use `addModelInstance(...)` to spawn visible objects in the scene.

#### Case A: use the mesh's own `.mtl` textures

This is the normal case for textured assets such as `prometheus.obj`.

```cpp
int ship_obj = renderer.addModelInstance(
    ship,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    -1
);
```

`texture_layer = -1` now means:

- use the mesh part's default material texture if one exists
- otherwise use the mesh part's material color

#### Case B: override the whole instance with one manual texture

```cpp
int sun_obj = renderer.addModelInstance(
    sphere,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{2.0f, 2.0f, 2.0f},
    sun_tex
);
```

That applies the same loaded override texture to every material part of that instance.

#### Case C: use explicit material control

```cpp
InstanceMaterial material;
material.texture_layer = -1;
material.tint = glm::vec4{1.0f, 0.8f, 0.8f, 1.0f};

int ship_obj = renderer.addModelInstance(
    ship,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    material
);
```

The tint multiplies the final sampled material color.

Return value:

- non-negative value: valid `object_id`
- `-1`: invalid `model_id`

### 6. Build camera matrices

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

### 7. Draw every frame

Per-frame usage:

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

renderer.updateInstanceMatrices();
renderer.draw(view, projection);

griddraw(view, projection);
SDL_GL_SwapWindow(window);
```

What each step does:

- `updateInstanceMatrices()`: rebuilds model matrices for dirty objects
- `draw(view, projection)`: draws all model instances

`uploadInstanceData()` still exists for compatibility, but it is currently a no-op. You can leave old calls in place, but they are no longer required.

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

## Per-Instance Materials

Each scene instance has both transform and material data.

Current material fields:

- `texture_layer`
- `tint`

### Update an object's material

```cpp
InstanceMaterial material = renderer.getInstanceMaterial(0, object_id);
material.tint = glm::vec4{1.0f, 0.4f, 0.4f, 1.0f};
renderer.setInstanceMaterial(0, object_id, material);
```

### Use the mesh's `.mtl` textures with a tint

```cpp
InstanceMaterial material;
material.texture_layer = -1;
material.tint = glm::vec4{1.0f, 1.0f, 0.8f, 1.0f};
renderer.setInstanceMaterial(0, object_id, material);
```

### Override the instance with one manually loaded texture

```cpp
InstanceMaterial material;
material.texture_layer = sun_tex;
material.tint = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
renderer.setInstanceMaterial(0, object_id, material);
```

Current shader behavior:

- if `texture_layer >= 0`, the instance uses that override texture
- if `texture_layer < 0`, the instance uses the mesh part's `.mtl` texture when present
- if neither exists, the mesh part falls back to its material color
- final output is multiplied by `tint`

Current limitation:

- there is no dedicated API for "ignore this textured mesh's `.mtl` texture and draw flat tint only"

## Working With Multiple Models And Textures

You can load multiple models and optional override textures:

```cpp
ObjRenderer renderer;
renderer.init();

int sun_model = renderer.loadMesh("assets/check.obj");
int earth_model = renderer.loadMesh("assets/earth.obj");

int sun_tex = renderer.loadTexture("assets/map/8k_sun.png");
int earth_tex = renderer.loadTexture("assets/map/8k_earth_daymap.jpg");
```

Then instantiate them by id:

```cpp
int sun = renderer.addModelInstance(
    sun_model,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{2.0f, 2.0f, 2.0f},
    sun_tex
);

int earth = renderer.addModelInstance(
    earth_model,
    glm::vec3{10.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    earth_tex
);
```

This means:

- `sun` uses `assets/check.obj` with the loaded `sun_tex` override
- `earth` uses `assets/earth.obj` with the loaded `earth_tex` override

If you instead want an OBJ's own `.mtl` textures, pass `-1`.

## Adding A Skybox

### To add a skybox with 6 png files

```cpp
initskybox({
    "assets/+X",
    "assets/-X",
    "assets/+Y",
    "assets/-Y",
    "assets/+Z",
    "assets/-Z",
});
```

Then in the render loop add:

```cpp
drawskybox(view, projection);
```

## Clearing The Current Scene

If you want to remove the currently spawned objects but keep the renderer, loaded meshes, and loaded textures alive, use:

```cpp
renderer.clearScene();
```

What `clearScene()` does:

- removes all current instances
- clears CPU-side instance storage
- keeps the loaded model assets alive
- keeps loaded `.mtl` textures and manually loaded override textures alive

Typical use:

```cpp
renderer.clearScene();

renderer.addModelInstance(0, glm::vec3{0, 0, 0}, glm::vec3{1, 1, 1}, -1);
renderer.addModelInstance(1, glm::vec3{5, 0, 0}, glm::vec3{2, 2, 2}, -1);
```

If you want to replace everything, including loaded meshes and textures, call:

```cpp
renderer.shutdown();
renderer.init();
```

Then load new meshes and textures again.

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
    -1
);

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
renderer.draw(view, projection);
```

Note:

- `clearScene()` removes render instances only
- if you are also using physics, you need to clear or rebuild your physics bodies separately before reusing old `object_id` values

## Minimal Example

This is the smallest useful renderer flow with an OBJ that carries its own `.mtl` textures:

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
if (!renderer.init()) {
  return -1;
}

int model = renderer.loadMesh(
    "assets/4c275vutixts-PrometheusNX59650/Prometheus NX 59650/prometheus.obj");
if (model < 0) {
  return -1;
}

int object_id = renderer.addModelInstance(
    model,
    glm::vec3{0.0f, 0.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 1.0f},
    -1
);

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
renderer.draw(view, projection);
SDL_GL_SwapWindow(window);
```

## Common Mistakes

- Initializing `ObjRenderer` before the OpenGL context exists.
- Expecting `init()` to load meshes or textures. It does not.
- Loading an OBJ with `map_Kd` textures and then manually reloading those same textures for no reason.
- Passing an invalid `model_id` to `addModelInstance(...)`.
- Mixing up `model_id` and `object_id`.
- Assuming `texture_layer = -1` means "flat untextured color" for every mesh. It now means "use the mesh default material".
- Forgetting to call `updateInstanceMatrices()` after changing transforms and before drawing.
- Calling `clearScene()` and then reusing stale physics body links or old `object_id` values.

## Where To Look Next

- Usage and integration example: [src/main.cpp](/home/plop/projects/mygame/src/main.cpp)
- Public renderer API: [headers/obj.h](/home/plop/projects/mygame/headers/obj.h)
- Renderer implementation: [src/objrender.cpp](/home/plop/projects/mygame/src/objrender.cpp)
- File-by-file documentation: [other_info.md](/home/plop/projects/mygame/other_info.md)
