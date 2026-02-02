# Chapter 2: Gravitational N-Body Simulation

**Author: Takao**

## Introduction

This chapter explains how to implement a GPU-based Gravitational N-Body Simulation - a method for simulating the motion of celestial bodies in space.

**Sample Project**: `Assets/NBodySimulation` in the Unity Graphics Programming 3 repository.

## What is N-Body Simulation?

N-Body simulation is a general term for simulations that calculate interactions between N physical objects. There are many types of problems that use N-Body simulation, and specifically, problems dealing with celestial bodies attracting each other through gravity in space are called **gravitational many-body problems**.

The algorithm explained in this chapter falls into this category - using N-Body simulation to solve the equations of motion for gravitational many-body systems.

### Applications Beyond Gravity

N-Body simulation is applied across many fields:
- Molecular force calculations
- Dark matter analysis
- Galaxy cluster collision analysis

From the smallest particles to cosmic scales, the technique has broad applications.

## The Algorithm

### Vector Form of Universal Gravitation

The basic universal gravitation equation from physics:

$$f = G \frac{Mm}{r^2}$$

This gives only the magnitude (scalar) of gravitational force between two bodies. For 3D simulation, we need the vector form.

The force vector that body *i* receives from body *j*:

$$\mathbf{f}_{ij} = G \frac{m_i m_j}{\|\mathbf{r}_{ij}\|^2} \cdot \frac{\mathbf{r}_{ij}}{\|\mathbf{r}_{ij}\|}$$

Where:
- $\mathbf{f}_{ij}$ = force vector on body *i* from body *j*
- $m_i, m_j$ = masses of the two bodies
- $\mathbf{r}_{ij}$ = direction vector from body *j* to body *i*

The left part calculates magnitude; the right part provides the unit direction vector.

### Total Force on a Single Body

The total force $\mathbf{F}_i$ on body *i* from all other bodies:

$$\mathbf{F}_i = \sum_{j \in N} \mathbf{f}_{ij} = Gm_i \cdot \sum_{j \in N} \frac{m_j \mathbf{r}_{ij}}{\|\mathbf{r}_{ij}\|^3}$$

### Softening Factor

To simplify simulation and handle collisions, we introduce a softening factor $\varepsilon$:

$$\mathbf{F}_i \simeq Gm_i \cdot \sum_{j \in N} \frac{m_j \mathbf{r}_{ij}}{(\|\mathbf{r}_{ij}\|^2 + \varepsilon)^{3/2}}$$

This prevents division by zero when particles are at the same position (including self-interaction, which yields 0).

### Converting Force to Acceleration

Using Newton's second law $m\mathbf{a} = \mathbf{f}$:

$$\mathbf{a}_i = \frac{\mathbf{F}_i}{m_i}$$

Substituting into our equation:

$$\mathbf{a}_i \simeq G \cdot \sum_{j \in N} \frac{m_j \mathbf{r}_{ij}}{(\|\mathbf{r}_{ij}\|^2 + \varepsilon)^{3/2}}$$

This is our final equation for computing acceleration.

## Finite Difference Method

### Understanding Differentiation

The mathematical definition of differentiation:

$$\frac{df}{dt} = \lim_{\Delta t \to 0} \frac{f(t + \Delta t) - f(t)}{\Delta t}$$

In physics, position, velocity, and acceleration are related:
- Velocity = derivative of position
- Acceleration = derivative of velocity

### Finite Differences for Computers

Computers cannot represent infinity, so we approximate with small finite $\Delta t$:

$$\frac{df}{dt} \simeq \frac{f(t + \Delta t) - f(t)}{\Delta t}$$

For position and velocity:

$$\frac{dx}{dt} \simeq \frac{x(t + \Delta t) - x(t)}{\Delta t} = v(t)$$

$$\frac{dv}{dt} \simeq \frac{v(t + \Delta t) - v(t)}{\Delta t} = a(t)$$

### The Update Equation

Combining these:

$$x(t + \Delta t) = x(t) + v(t + \Delta t) \Delta t = x(t) + (v(t) + a(t) \Delta t) \Delta t$$

This means: **knowing current position, velocity, and acceleration, we can compute the position at time $t + \Delta t$**.

For real-time simulation, $\Delta t$ is typically the frame time (1/60 second at 60fps).

## Implementation

### Data Structure

```csharp
public struct Body
{
    public Vector3 position;
    public Vector3 velocity;
    public float mass;
}
```

### Buffer Initialization

```csharp
void InitBuffer()
{
    // Create Read/Write buffers to prevent race conditions
    bodyBuffers = new ComputeBuffer[2];

    bodyBuffers[READ] = new ComputeBuffer(numBodies,
        Marshal.SizeOf(typeof(Body)));

    bodyBuffers[WRITE] = new ComputeBuffer(numBodies,
        Marshal.SizeOf(typeof(Body)));
}
```

### Initial Particle Distribution

```csharp
void DistributeBodies()
{
    Random.InitState(seed);
    float scale = positionScale * Mathf.Max(1, numBodies / DEFAULT_PARTICLE_NUM);

    Body[] bodies = new Body[numBodies];

    int i = 0;
    while (i < numBodies)
    {
        // Random sampling within a sphere
        Vector3 pos = Random.insideUnitSphere;

        bodies[i].position = pos * scale;
        bodies[i].velocity = Vector3.zero;
        bodies[i].mass = Random.Range(0.1f, 1.0f);

        i++;
    }

    bodyBuffers[READ].SetData(bodies);
    bodyBuffers[WRITE].SetData(bodies);
}
```

### Simulation Loop

```csharp
void Update()
{
    // Set constants
    NBodyCS.SetFloat("_DeltaTime", Time.deltaTime);
    NBodyCS.SetFloat("_Damping", damping);
    NBodyCS.SetFloat("_SofteningSquared", softeningSquared);
    NBodyCS.SetInt("_NumBodies", numBodies);

    // Thread dimensions
    NBodyCS.SetVector("_ThreadDim",
        new Vector4(SIMULATION_BLOCK_SIZE, 1, 1, 0));

    NBodyCS.SetVector("_GroupDim",
        new Vector4(Mathf.CeilToInt(numBodies / SIMULATION_BLOCK_SIZE), 1, 1, 0));

    // Set buffers
    NBodyCS.SetBuffer(0, "_BodiesBufferRead", bodyBuffers[READ]);
    NBodyCS.SetBuffer(0, "_BodiesBufferWrite", bodyBuffers[WRITE]);

    // Execute
    NBodyCS.Dispatch(0,
        Mathf.CeilToInt(numBodies / SIMULATION_BLOCK_SIZE), 1, 1);

    // Swap buffers
    Swap(bodyBuffers);
}
```

### Rendering

```csharp
void OnRenderObject()
{
    particleRenderMat.SetPass(0);
    particleRenderMat.SetBuffer("_Particles", bodyBuffers[READ]);
    Graphics.DrawProcedural(MeshTopology.Points, numBodies);
}
```

## GPU Implementation with Shared Memory

N-Body simulation requires calculating interactions between all particles, resulting in O(n²) complexity. Using shared memory (from Unity Graphics Programming Vol.1, Chapter 3) dramatically improves performance.

### Tiled Approach Concept

Particles within the same thread block share data via shared memory, reducing global memory I/O. The concept:

1. Each row represents a global thread (DispatchThreadID)
2. Each column represents particles being examined
3. Tiles process blocks of particles at a time
4. All rows run in parallel but synchronize within tiles

With 256 threads per block (SIMULATION_BLOCK_SIZE), each tile is 256x256.

### Compute Shader Constants

```hlsl
#include "Body.cginc"

cbuffer cb {
    float _SofteningSquared, _DeltaTime, _Damping;
    uint _NumBodies;
    float4 _GroupDim, _ThreadDim;
};

StructuredBuffer<Body> _BodiesBufferRead;
RWStructuredBuffer<Body> _BodiesBufferWrite;

// Shared memory for the thread block
groupshared Body sharedBody[SIMULATION_BLOCK_SIZE];
```

### Tile Implementation

```hlsl
float3 computeBodyForce(Body body, uint3 groupID, uint3 threadID)
{
    uint start = 0;
    uint finish = _NumBodies;

    float3 acc = (float3)0;
    int currentTile = 0;

    // Process each tile (block)
    for (uint i = start; i < finish; i += SIMULATION_BLOCK_SIZE)
    {
        // Load into shared memory
        sharedBody[threadID.x]
            = _BodiesBufferRead[wrap(groupID.x + currentTile, _GroupDim.x)
                * SIMULATION_BLOCK_SIZE + threadID.x];

        // Synchronize within group
        GroupMemoryBarrierWithGroupSync();

        // Calculate gravitational effects
        acc = gravitation(body, acc, threadID);

        GroupMemoryBarrierWithGroupSync();

        currentTile++;
    }

    return acc;
}
```

### Gravitational Interaction

```hlsl
float3 gravitation(Body body, float3 accel, uint3 threadID)
{
    // Process all bodies in the tile
    for (uint i = 0; i < SIMULATION_BLOCK_SIZE;)
    {
        accel = bodyBodyInteraction(accel, sharedBody[i], body);
        i++;
    }
    return accel;
}

// Core gravitational calculation
float3 bodyBodyInteraction(float3 acc, Body b_i, Body b_j)
{
    float3 r = b_i.position - b_j.position;

    // distSqr = dot(r_ij, r_ij) + EPS^2
    float distSqr = r.x * r.x + r.y * r.y + r.z * r.z;
    distSqr += _SofteningSquared;

    // invDistCube = 1/distSqr^(3/2)
    float distSixth = distSqr * distSqr * distSqr;
    float invDistCube = 1.0f / sqrt(distSixth);

    // s = m_j * invDistCube
    float s = b_j.mass * invDistCube;

    // a_i = a_i + s * r_ij
    acc += r * s;

    return acc;
}
```

### Position Update (Main Kernel)

```hlsl
[numthreads(SIMULATION_BLOCK_SIZE,1,1)]
void CSMain (
    uint3 groupID : SV_GroupID,
    uint3 threadID : SV_GroupThreadID,
    uint3 DTid : SV_DispatchThreadID
) {
    uint index = DTid.x;
    Body body = _BodiesBufferRead[index];

    float3 force = computeBodyForce(body, groupID, threadID);

    body.velocity += force * _DeltaTime;
    body.velocity *= _Damping;

    // Finite difference update
    body.position += body.velocity * _DeltaTime;

    _BodiesBufferWrite[index] = body;
}
```

## Billboard Particle Rendering

### What is a Billboard?

A billboard is a simple plane object that always faces the camera. Most particle systems use billboards for rendering.

### Implementation with View Matrix

The view matrix contains camera position and rotation information. To make a billboard face the camera, we apply the **inverse of the view matrix's rotation component** as the model transformation.

### Geometry Shader for Quad Expansion

```hlsl
[maxvertexcount(4)]
void geom(point v2g input[1], inout TriangleStream<g2f> outStream) {
    g2f o;
    float4 pos = input[0].pos;

    float4x4 billboardMatrix = UNITY_MATRIX_V;

    // Extract rotation only (zero out translation)
    billboardMatrix._m03 = billboardMatrix._m13 =
        billboardMatrix._m23 = billboardMatrix._m33 = 0;

    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            float2 uv = float2(x, y);
            o.uv = uv;

            o.pos = pos
                + mul(transpose(billboardMatrix), float4((uv * 2 - float2(1, 1))
                * _Scale, 0, 1));

            o.pos = mul(UNITY_MATRIX_VP, o.pos);
            o.id = input[0].id;

            outStream.Append(o);
        }
    }
    outStream.RestartStrip();
}
```

## Making It Visually Interesting

The basic simulation tends to collapse all particles to the center, which isn't visually appealing. A simple trick: **truncate the interaction calculation early**:

```hlsl
float3 computeBodyForce(Body body, uint3 groupID, uint3 threadID)
{
    ...
    uint finish = _NumBodies / div;  // Cut off early
    ...
}
```

By not computing all interactions, particles form multiple clusters that interact with each other, creating more dynamic and interesting motion.

## Key Takeaways

1. **N-Body simulation** calculates gravitational interactions between all particles
2. **Finite difference method** discretizes differential equations for computer simulation
3. **Shared memory optimization** is essential for O(n²) algorithms on GPU
4. **Tile-based processing** reduces global memory access
5. **Billboard rendering** efficiently displays particles as camera-facing quads
6. **Artistic liberties** (like truncating calculations) can improve visual results
7. The technique has applications from **molecular simulation to galaxy formation**

## References

- GPU Gems 3 - Chapter 31: Fast N-Body Simulation with CUDA
- N-Body Simulation Research (Kagoshima University)
- Quaternions and Billboards (wgld.org)
