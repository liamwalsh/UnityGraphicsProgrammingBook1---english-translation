# Chapter 3: GPU Implementation of Flocking Simulation

*Original author: @hiroakioishi*
*Translation and annotations by Claude*

---

## Introduction

This chapter explains how to implement flocking simulation using the Boids algorithm with ComputeShaders.

Birds, fish, and other animals sometimes form flocks or schools. The movement of these groups exhibits both regularity and complexity, possessing a certain beauty that has captivated people throughout history.

In computer graphics, controlling the behavior of each individual manually is impractical. To address this, an algorithm called **Boids** was devised for creating flocking behavior. This simulation algorithm consists of several simple rules and is easy to implement. However, a naive implementation requires checking the positional relationships with all other individuals, causing computational costs to increase with the square of the population size. When you want to control many individuals, CPU-based implementation becomes extremely difficult.

This is where we leverage the GPU's powerful parallel computing capabilities. Unity provides ComputeShaders for this kind of general-purpose GPU computing (GPGPU). GPUs have a special memory region called **shared memory**, and ComputeShaders allow us to effectively utilize this memory. Additionally, Unity has an advanced rendering feature called **GPU Instancing** that enables drawing large quantities of arbitrary meshes. This chapter introduces a program that uses these Unity GPU features to control and render many Boid objects.

> **💡 Why This Matters**
>
> The Boids algorithm is a classic example of **emergent behavior**—complex, lifelike patterns arising from simple rules. It's used in:
> - Film and games (flocking birds, schools of fish, crowd simulation)
> - Screensavers and generative art
> - Robotics research (swarm intelligence)
>
> Understanding this chapter will teach you both the algorithm AND important GPU optimization techniques.

---

## The Boids Algorithm

The Boids flocking simulation algorithm was developed by Craig Reynolds in 1986 and presented the following year at ACM SIGGRAPH 1987 in a paper titled "Flocks, Herds, and Schools: A Distributed Behavioral Model."

Reynolds observed that flocking behavior emerges when each individual modifies its own actions based on the positions and movement directions of surrounding individuals, perceived through senses like sight and hearing.

Each individual follows these **three simple behavioral rules**:

### 1. Separation

Move to avoid crowding with other individuals within a certain distance.

> **💡 Think of it as:** "Don't crash into your neighbors"

### 2. Alignment

Move toward the average heading direction of other individuals within a certain distance.

> **💡 Think of it as:** "Fly in the same direction as your neighbors"

### 3. Cohesion

Move toward the average position of other individuals within a certain distance.

> **💡 Think of it as:** "Stay close to your neighbors"

![Boids Basic Rules](images/boids-rules.png)
*Figure: The three fundamental Boids rules*

By controlling individual movements according to these rules, we can program flocking behavior.

> **💡 The Magic of Emergence**
>
> What's remarkable is that NO individual knows about the "flock" as a whole. Each agent only follows local rules based on nearby neighbors. Yet from these simple rules, complex, organic-looking group behavior emerges. This is the essence of emergent systems.

---

## Sample Program

### Repository

https://github.com/IndieVisualLab/UnityGraphicsProgramming

Open the **BoidsSimulationOnGPU.unity** scene in the Assets/**BoidsSimulationOnGPU** folder of the sample Unity project.

### Runtime Requirements

This program uses ComputeShaders and GPU Instancing.

**ComputeShader** works on the following platforms/APIs:
- Windows and Windows Store apps with DirectX 11 or DirectX 12 graphics API and Shader Model 5.0 GPU
- macOS and iOS using Metal graphics API
- Android, Linux, and Windows platforms with Vulkan API
- Modern OpenGL platforms (OpenGL 4.3 on Linux/Windows, OpenGL ES 3.1 on Android). Note: macOS does not support OpenGL 4.3
- Current-generation consoles (Sony PS4, Microsoft Xbox One)

**GPU Instancing** is available on:
- DirectX 11 and DirectX 12 on Windows
- OpenGL Core 4.1+/ES3.0+ on Windows, macOS, Linux, iOS, Android
- Metal on macOS and iOS
- Vulkan on Windows and Android
- PlayStation 4 and Xbox One
- WebGL (requires WebGL 2.0 API)

This sample uses the `Graphics.DrawMeshInstancedIndirect` method, requiring Unity 5.6 or later.

---

## Implementation Code Explanation

The sample program consists of the following code:

| File | Purpose |
|------|---------|
| **GPUBoids.cs** | C# script that controls the ComputeShader for Boids simulation |
| **Boids.compute** | ComputeShader that performs the Boids simulation |
| **BoidsRender.cs** | C# script that controls the rendering shader |
| **BoidsRender.shader** | Shader for drawing objects via GPU instancing |

![Unity Editor Setup](images/editor-boids.png)
*Figure: Settings in the Unity Editor*

---

## GPUBoids.cs - The Simulation Controller

This code manages the Boids simulation parameters, buffers needed for GPU computation, and the ComputeShader containing the calculation instructions.

### Key Structure: BoidData

```csharp
// Boid data structure
[System.Serializable]
struct BoidData
{
    public Vector3 Velocity; // Velocity
    public Vector3 Position; // Position
}
```

> **💡 Why a struct?**
>
> GPU compute buffers work with contiguous memory blocks. By defining a struct, we ensure all boid data is laid out predictably in memory, making GPU access efficient.

### Simulation Parameters

```csharp
// Thread group thread size
const int SIMULATION_BLOCK_SIZE = 256;

// Maximum object count
[Range(256, 32768)]
public int MaxObjectNum = 16384;

// Radius for applying cohesion with other individuals
public float CohesionNeighborhoodRadius  = 2.0f;
// Radius for applying alignment with other individuals
public float AlignmentNeighborhoodRadius = 2.0f;
// Radius for applying separation with other individuals
public float SeparateNeighborhoodRadius  = 1.0f;

// Maximum velocity
public float MaxSpeed        = 5.0f;
// Maximum steering force
public float MaxSteerForce   = 0.5f;

// Weight for cohesion force
public float CohesionWeight  = 1.0f;
// Weight for alignment force
public float AlignmentWeight = 1.0f;
// Weight for separation force
public float SeparateWeight  = 3.0f;

// Weight for wall avoidance force
public float AvoidWallWeight = 10.0f;
```

> **💡 Parameter Tuning**
>
> These weights dramatically affect behavior:
> - **High SeparateWeight**: Individuals spread out, avoid crowding
> - **High CohesionWeight**: Tight, dense flocks
> - **High AlignmentWeight**: Smooth, coordinated movement
>
> Try adjusting these in the Unity Inspector to see different flocking behaviors!

### ComputeBuffer Initialization

The `InitBuffer` function declares the buffers used for GPU computation.

```csharp
void InitBuffer()
{
    // Initialize buffers
    _boidDataBuffer  = new ComputeBuffer(MaxObjectNum,
        Marshal.SizeOf(typeof(BoidData)));
    _boidForceBuffer = new ComputeBuffer(MaxObjectNum,
        Marshal.SizeOf(typeof(Vector3)));

    // Initialize Boid data and Force buffers
    var forceArr = new Vector3[MaxObjectNum];
    var boidDataArr = new BoidData[MaxObjectNum];
    for (var i = 0; i < MaxObjectNum; i++)
    {
        forceArr[i] = Vector3.zero;
        boidDataArr[i].Position = Random.insideUnitSphere * 1.0f;
        boidDataArr[i].Velocity = Random.insideUnitSphere * 0.1f;
    }
    _boidForceBuffer.SetData(forceArr);
    _boidDataBuffer.SetData(boidDataArr);
}
```

**ComputeBuffer** is a data buffer for storing data for ComputeShaders. It allows reading and writing to GPU memory buffers from C# scripts. The constructor arguments are the number of buffer elements and the size (in bytes) of each element. Using `Marshal.SizeOf()` retrieves the byte size of a type. With `SetData()`, you can set values from an array of any struct.

### Executing ComputeShader Functions

The `Simulation` function passes necessary parameters to the ComputeShader and issues computation commands.

Functions in ComputeShaders that actually perform GPU calculations are called **kernels**. The execution unit for kernels is called a **thread**, and for parallel processing suited to GPU architecture, threads are grouped together into **thread groups**.

The product of thread count and thread group count should equal or exceed the number of Boid objects.

```csharp
void Simulation()
{
    ComputeShader cs = BoidsCS;
    int id = -1;

    // Calculate the number of thread groups
    int threadGroupSize = Mathf.CeilToInt(MaxObjectNum / SIMULATION_BLOCK_SIZE);

    // Calculate steering force
    id = cs.FindKernel("ForceCS"); // Get kernel ID
    cs.SetInt("_MaxBoidObjectNum", MaxObjectNum);
    cs.SetFloat("_CohesionNeighborhoodRadius", CohesionNeighborhoodRadius);
    // ... (set other parameters)
    cs.SetBuffer(id, "_BoidDataBufferRead", _boidDataBuffer);
    cs.SetBuffer(id, "_BoidForceBufferWrite", _boidForceBuffer);
    cs.Dispatch(id, threadGroupSize, 1, 1); // Execute ComputeShader

    // Calculate velocity and position from steering force
    id = cs.FindKernel("IntegrateCS"); // Get kernel ID
    cs.SetFloat("_DeltaTime", Time.deltaTime);
    cs.SetBuffer(id, "_BoidForceBufferRead", _boidForceBuffer);
    cs.SetBuffer(id, "_BoidDataBufferWrite", _boidDataBuffer);
    cs.Dispatch(id, threadGroupSize, 1, 1); // Execute ComputeShader
}
```

Kernels are specified using the `#pragma kernel` directive in the ComputeShader script. Each is assigned an ID, which can be retrieved from C# using `FindKernel`.

The `Dispatch` method issues a command to perform GPU computation on the kernel defined in the ComputeShader. Arguments include the kernel ID and the number of thread groups.

---

## Boids.compute - The GPU Computation

This file contains the GPU computation instructions. There are two kernels: one calculates steering forces, and the other applies those forces to update velocity and position.

### Buffer Declarations

```hlsl
// Kernel function specification
#pragma kernel ForceCS      // Calculate steering force
#pragma kernel IntegrateCS  // Calculate velocity, position

// Boid data structure
struct BoidData
{
    float3 velocity; // Velocity
    float3 position; // Position
};

// Thread group thread size
#define SIMULATION_BLOCK_SIZE 256

// Boid data buffer (read-only)
StructuredBuffer<BoidData>   _BoidDataBufferRead;
// Boid data buffer (read-write)
RWStructuredBuffer<BoidData> _BoidDataBufferWrite;
// Boid steering force buffer (read-only)
StructuredBuffer<float3>     _BoidForceBufferRead;
// Boid steering force buffer (read-write)
RWStructuredBuffer<float3>   _BoidForceBufferWrite;
```

> **💡 Read vs Read-Write Buffers**
>
> - `StructuredBuffer<T>`: Read-only. Use when data won't change during the kernel.
> - `RWStructuredBuffer<T>`: Read-write. Use when you need to write results.
>
> This separation helps the GPU optimize memory access patterns.

### Shared Memory - The Key Optimization

```hlsl
// Shared memory for Boid data
groupshared BoidData boid_data[SIMULATION_BLOCK_SIZE];
```

Variables with the `groupshared` storage modifier are written to **shared memory**. Shared memory can't hold large amounts of data, but it's placed close to registers and provides very fast access. This shared memory is accessible within a thread group.

By writing SIMULATION_BLOCK_SIZE worth of other individuals' data to shared memory at once and allowing fast reading within the same thread group, we can efficiently perform calculations that consider positional relationships with other individuals.

![GPU Architecture](images/gpu-architecture.png)
*Figure: Basic GPU architecture*

> **💡 Why Shared Memory Matters**
>
> Without this optimization, each thread would read other boids' data from slow global memory thousands of times. By loading a batch into fast shared memory and reusing it across the thread group, we get massive speedups.
>
> This is a fundamental GPU optimization pattern called **tiling**.

### GroupMemoryBarrierWithGroupSync()

When accessing data written to shared memory, you must write `GroupMemoryBarrierWithGroupSync()` to synchronize all threads in the thread group.

This function blocks execution of all threads in the group until all threads have reached this call. This ensures that initialization of the `boid_data` array is properly completed across all threads in the thread group.

### The Force Calculation Kernel

```hlsl
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ForceCS
(
    uint3 DTid : SV_DispatchThreadID, // Unique ID across all threads
    uint3 Gid : SV_GroupID,           // Group ID
    uint3 GTid : SV_GroupThreadID,    // Thread ID within group
    uint  GI : SV_GroupIndex          // SV_GroupThreadID as 1D (0-255)
)
{
    const unsigned int P_ID = DTid.x;
    float3 P_position = _BoidDataBufferRead[P_ID].position;
    float3 P_velocity = _BoidDataBufferRead[P_ID].velocity;

    float3 force = float3(0, 0, 0);

    // Variables for calculating the three rules
    float3 sepPosSum = float3(0, 0, 0); int sepCount = 0;
    float3 aliVelSum = float3(0, 0, 0); int aliCount = 0;
    float3 cohPosSum = float3(0, 0, 0); int cohCount = 0;

    // Process in blocks of SIMULATION_BLOCK_SIZE
    [loop]
    for (uint N_block_ID = 0; N_block_ID < (uint)_MaxBoidObjectNum;
        N_block_ID += SIMULATION_BLOCK_SIZE)
    {
        // Load SIMULATION_BLOCK_SIZE boids into shared memory
        boid_data[GI] = _BoidDataBufferRead[N_block_ID + GI];

        // Synchronize threads
        GroupMemoryBarrierWithGroupSync();

        // Calculate with other individuals
        for (int N_tile_ID = 0; N_tile_ID < SIMULATION_BLOCK_SIZE; N_tile_ID++)
        {
            float3 N_position = boid_data[N_tile_ID].position;
            float3 N_velocity = boid_data[N_tile_ID].velocity;

            float3 diff = P_position - N_position;
            float  dist = sqrt(dot(diff, diff));

            // --- Separation ---
            if (dist > 0.0 && dist <= _SeparateNeighborhoodRadius)
            {
                float3 repulse = normalize(P_position - N_position);
                repulse /= dist; // Closer = stronger repulsion
                sepPosSum += repulse;
                sepCount++;
            }

            // --- Alignment ---
            if (dist > 0.0 && dist <= _AlignmentNeighborhoodRadius)
            {
                aliVelSum += N_velocity;
                aliCount++;
            }

            // --- Cohesion ---
            if (dist > 0.0 && dist <= _CohesionNeighborhoodRadius)
            {
                cohPosSum += N_position;
                cohCount++;
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // Calculate steering forces from accumulated values
    // ... (steering force calculations)

    _BoidForceBufferWrite[P_ID] = force;
}
```

> **💡 Understanding the Tiling Pattern**
>
> The outer loop processes boids in "tiles" of 256 at a time:
> 1. Load 256 boids from global memory into shared memory
> 2. Wait for all threads to finish loading
> 3. Each thread compares itself against all 256 loaded boids
> 4. Repeat for the next 256 boids
>
> This converts O(N) global memory reads into O(N/256) global reads + O(N) fast shared memory reads.

### The Integration Kernel

```hlsl
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void IntegrateCS(uint3 DTid : SV_DispatchThreadID)
{
    const unsigned int P_ID = DTid.x;

    BoidData b = _BoidDataBufferWrite[P_ID];
    float3 force = _BoidForceBufferRead[P_ID];

    // Apply repulsion when approaching walls
    force += avoidWall(b.position) * _AvoidWallWeight;

    b.velocity += force * _DeltaTime; // Apply force to velocity
    b.velocity = limit(b.velocity, _MaxSpeed); // Limit velocity
    b.position += b.velocity * _DeltaTime; // Update position

    _BoidDataBufferWrite[P_ID] = b;
}
```

---

## GPU Instancing for Rendering

When you want to draw large quantities of identical meshes, creating individual GameObjects increases draw calls and rendering load. Additionally, transferring ComputeShader results to CPU memory is costly. For high-speed processing, we need to pass GPU computation results directly to the rendering shader.

Unity's GPU Instancing allows drawing large quantities of identical meshes with minimal draw calls without creating unnecessary GameObjects.

### Graphics.DrawMeshInstancedIndirect()

The **BoidsRender.cs** script uses `Graphics.DrawMeshInstancedIndirect` for GPU-instanced mesh rendering. This method allows passing mesh index counts and instance counts as ComputeBuffers, which is convenient when you want to read all instance data from the GPU.

```csharp
void RenderInstancedMesh()
{
    // Get index count of specified mesh
    uint numIndices = (InstanceMesh != null) ?
        (uint)InstanceMesh.GetIndexCount(0) : 0;
    args[0] = numIndices;
    args[1] = (uint)GPUBoidsScript.GetMaxObjectNum();
    argsBuffer.SetData(args);

    // Set Boid data buffer to material
    InstanceRenderMaterial.SetBuffer("_BoidDataBuffer",
        GPUBoidsScript.GetBoidDataBuffer());
    InstanceRenderMaterial.SetVector("_ObjectScale", ObjectScale);

    // Define bounds
    var bounds = new Bounds(
        GPUBoidsScript.GetSimulationAreaCenter(),
        GPUBoidsScript.GetSimulationAreaSize()
    );

    // Draw mesh with GPU instancing
    Graphics.DrawMeshInstancedIndirect(
        InstanceMesh,           // Mesh to instance
        0,                      // Submesh index
        InstanceRenderMaterial, // Rendering material
        bounds,                 // Bounds
        argsBuffer              // Arguments buffer
    );
}
```

> **💡 The Power of Indirect Drawing**
>
> `DrawMeshInstancedIndirect` is special because the instance count comes from a GPU buffer, not a CPU variable. This means:
> - No CPU-GPU synchronization needed
> - Instance count can be determined by GPU computation
> - Perfect for particle systems, procedural content, etc.

---

## The Rendering Shader

The **BoidsRender.shader** is designed to work with `Graphics.DrawMeshInstancedIndirect`.

### Key Shader Setup

```hlsl
#pragma surface surf Standard vertex:vert addshadow
#pragma instancing_options procedural:setup
```

The `procedural:setup` directive tells Unity to generate additional variants for use with `Graphics.DrawMeshInstancedIndirect`. The specified function (`setup`) is called at the beginning of the vertex shader stage.

### Vertex Shader - Transforming Each Boid

```hlsl
void vert(inout appdata_full v)
{
    #ifdef UNITY_PROCEDURAL_INSTANCING_ENABLED
    // Get Boid data from instance ID
    BoidData boidData = _BoidDataBuffer[unity_InstanceID];

    float3 pos = boidData.position.xyz;
    float3 scl = _ObjectScale;

    // Create object-to-world transformation matrix
    float4x4 object2world = (float4x4)0;
    object2world._11_22_33_44 = float4(scl.xyz, 1.0);

    // Calculate rotation from velocity
    float rotY = atan2(boidData.velocity.x, boidData.velocity.z);
    float rotX = -asin(boidData.velocity.y /
        (length(boidData.velocity.xyz) + 1e-8));

    float4x4 rotMatrix = eulerAnglesToRotationMatrix(float3(rotX, rotY, 0));
    object2world = mul(rotMatrix, object2world);
    object2world._14_24_34 += pos.xyz;

    // Transform vertex and normal
    v.vertex = mul(object2world, v.vertex);
    v.normal = normalize(mul(object2world, v.normal));
    #endif
}
```

Using `unity_InstanceID`, you can get a unique ID for each instance. By using this ID as an index into the StructuredBuffer declared as Boid data buffer, you can obtain unique Boid data for each instance.

### Calculating Rotation from Velocity

The rotation is calculated from the Boid's velocity data so it faces its direction of travel. Using Euler angles for intuitive handling:

- **Yaw** (horizontal direction): Calculated using `atan2` from Z and X velocity components
- **Pitch** (vertical tilt): Calculated using `asin` from the ratio of Y velocity to total velocity magnitude

![Roll Pitch Yaw](images/roll-pitch-yaw.png)
*Figure: Axis rotation terminology*

---

## Results

With this implementation, you should see objects that move with flock-like behavior.

![Execution Result](images/result.png)
*Figure: Execution result*

---

## Summary

The implementation introduced in this chapter uses the minimum Boids algorithm, but through parameter adjustment, flocks can form large groups or split into many smaller groups, showing different movement characteristics.

Beyond the basic behavioral rules shown here, other rules should be considered. For example, if these were fish and a predator appeared, they would naturally flee. If there were terrain or obstacles, the fish would avoid collisions. Regarding vision, different animal species have different fields of view and acuity—excluding individuals outside the field of view from calculations would make the simulation more realistic.

Whether flying through air, swimming in water, or moving on land, the environment and characteristics of locomotor organs also affect movement patterns. Individual differences should also be considered.

### Performance Considerations

GPU parallel processing can calculate many more individuals than CPU-based computation, but fundamentally, calculations with other individuals are done exhaustively (O(N²)), which isn't very efficient. To reduce computational cost, you could:
- Register individuals in grid or block regions based on their positions
- Only calculate with individuals in adjacent regions (spatial hashing)

This **neighbor search optimization** can significantly reduce computational costs for very large populations.

> **💡 Next Steps for Exploration**
>
> - Try different mesh shapes (fish, birds, abstract shapes)
> - Add predator/prey relationships
> - Implement obstacle avoidance
> - Experiment with spatial partitioning for better performance
> - Add visual trails or other effects

---

## References

- Boids Background and Update - https://www.red3d.com/cwr/boids/
- THE NATURE OF CODE - http://natureofcode.com/
- Real-Time Particle Systems on the GPU in Dynamic Environments
- Practical Rendering and Computation with Direct3D 11
- GPU Parallel Graphics Processing Introduction (Japanese)

---

## Key Takeaways

> **🎯 What You Should Remember**
>
> 1. **Boids = 3 simple rules** → Separation, Alignment, Cohesion
>
> 2. **GPU optimization with shared memory** → Load data in tiles, reuse across thread group
>
> 3. **GroupMemoryBarrierWithGroupSync()** → Essential for shared memory coordination
>
> 4. **GPU Instancing** → Draw thousands of meshes with minimal draw calls
>
> 5. **DrawMeshInstancedIndirect** → Instance count from GPU buffer, no CPU sync needed
>
> 6. **Vertex shader transformation** → Use `unity_InstanceID` to get per-instance data

---

*Next chapter: Grid-based Fluid Simulation with Stable Fluids!*
