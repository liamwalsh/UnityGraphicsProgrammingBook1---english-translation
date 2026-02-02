# Chapter 4: GPU-Based Cellular Growth Simulation

**Author: Nakamura (@mattatz)**

## Introduction

This chapter develops a GPU-based program for simulating cell division and growth, inspired by the "Cell Division and Growth Algorithm 1" tutorial from iGeo, a procedural modeling library for Processing used in architecture.

**Sample Project**: `CellularGrowth` in the Unity Graphics Programming 3 repository.

### Topics Covered

- Using Append/ConsumeStructuredBuffer for dynamic GPU object management
- Representing network structures on the GPU
- Sequential processing with atomic operations

## Cell Division and Growth Simulation

The simulation uses two structures: **Particle** and **Edge**.

### Particle Behaviors

Each Particle represents a cell with three behaviors:

1. **Growth**: Increases in size over time
2. **Repulsion**: Collides and repels other particles
3. **Division**: Splits into two particles under certain conditions

### Edge Purpose

Edges represent cell adhesion. When particles divide, edges connect them like springs, pulling particles together to form network structures.

## Implementation

### Particle Structure

```csharp
[StructLayout(LayoutKind.Sequential)]
public struct Particle_t {
    public Vector2 position;    // Position
    public Vector2 velocity;    // Velocity
    float radius;               // Current size
    float threshold;            // Maximum size
    int links;                  // Connected edge count
    uint alive;                 // Active flag
}
```

### Append/ConsumeStructuredBuffer

These are LIFO (Last In First Out) containers available since DirectX 11, enabling dynamic object count management on the GPU.

- **AppendStructuredBuffer**: Adds data
- **ConsumeStructuredBuffer**: Retrieves data

### Buffer Initialization

```csharp
protected void Start() {
    // Particle buffer (ping-pong for read/write separation)
    particleBuffer = new PingPongBuffer(count, typeof(Particle_t));

    // Object pool buffer
    poolBuffer = new ComputeBuffer(
        count,
        Marshal.SizeOf(typeof(int)),
        ComputeBufferType.Append
    );
    poolBuffer.SetCounterValue(0);

    // Count buffer for tracking pool size
    countBuffer = new ComputeBuffer(
        4,
        Marshal.SizeOf(typeof(int)),
        ComputeBufferType.IndirectArguments
    );

    // Dividable particles pool
    dividablePoolBuffer = new ComputeBuffer(
        count,
        Marshal.SizeOf(typeof(int)),
        ComputeBufferType.Append
    );

    InitParticlesKernel();
}
```

### Object Pool Pattern

The poolBuffer stores indices of inactive particles:
1. Initialize: Add all particle indices to pool (all inactive)
2. Spawn: Pop index from pool, activate that particle
3. Despawn: Push index back to pool, deactivate particle

### Particle Initialization Kernel

```hlsl
THREAD
void InitParticles(uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    uint count, strides;
    _Particles.GetDimensions(count, strides);
    if (idx >= count) return;

    // Initialize particle as inactive
    Particle p = create();
    p.alive = false;
    _Particles[idx] = p;

    // Add index to object pool
    _ParticlePoolAppend.Append(idx);
}
```

### Emitting Particles

```csharp
protected void EmitParticlesKernel(Vector2 point, int emitCount = 32)
{
    // Prevent consuming from empty pool
    emitCount = Mathf.Max(0, Mathf.Min(emitCount, CopyPoolSize(poolBuffer)));
    if (emitCount <= 0) return;

    var kernel = compute.FindKernel("EmitParticles");
    compute.SetBuffer(kernel, "_Particles", particleBuffer.Read);
    compute.SetBuffer(kernel, "_ParticlePoolConsume", poolBuffer);
    compute.SetVector("_Point", point);
    compute.SetInt("_EmitCount", emitCount);

    Dispatch1D(kernel, emitCount);
}
```

**Critical**: Always check pool size before consuming to avoid undefined behavior.

```hlsl
THREAD
void EmitParticles(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= (uint)_EmitCount) return;

    // Get inactive particle index from pool
    uint idx = _ParticlePoolConsume.Consume();

    Particle c = create();
    float2 offset = random_point_on_circle(id.xx + float2(0, _Time));
    c.position = _Point.xy + offset;
    c.radius = nrand(id.xx + float2(_Time, 0));

    _Particles[idx] = c;
}
```

### Particle Update (Growth + Repulsion)

```hlsl
THREAD
void UpdateParticles(uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    uint count, strides;
    _ParticlesRead.GetDimensions(count, strides);
    if (idx >= count) return;

    Particle p = _ParticlesRead[idx];

    if (p.alive)
    {
        // Growth: increase size up to threshold
        p.radius = min(p.threshold, p.radius + _DT * _Grow);

        // Repulsion: check collision with all other particles
        for (uint i = 0; i < count; i++)
        {
            Particle other = _ParticlesRead[i];
            if (i == idx || !other.alive) continue;

            float2 dir = p.position - other.position;
            float l = length(dir);
            float r = (p.radius + other.radius) * _Repulsion;

            if (l < r)
            {
                p.velocity += normalize(dir) * (r - l);
            }
        }

        // Apply velocity with drag
        float2 vel = p.velocity * _DT;
        float vl = length(vel);
        if (vl > 0)
        {
            p.position += normalize(vel) * min(vl, _Limit);
            p.velocity = normalize(p.velocity) *
                min(length(p.velocity) * _Drag, _Limit);
        }
    }

    _Particles[idx] = p;
}
```

### Ping-Pong Buffer Pattern

When threads read data modified by other threads (like collision detection), we need separate read and write buffers to prevent data races:

```csharp
// Read from one buffer, write to another
compute.SetBuffer(kernel, "_ParticlesRead", particleBuffer.Read);
compute.SetBuffer(kernel, "_Particles", particleBuffer.Write);

Dispatch1D(kernel, count);

// Swap buffers for next frame
particleBuffer.Swap();
```

### Division System

Division occurs periodically via coroutine:

```csharp
protected IEnumerator IDivider()
{
    yield return 0;
    while(true)
    {
        yield return new WaitForSeconds(divideInterval);
        Divide();
    }
}

protected void Divide() {
    GetDividableParticlesKernel();
    DivideParticlesKernel(maxDivideCount);
}
```

### Division Kernel

```hlsl
bool dividable_particle(Particle p, uint idx)
{
    // Divide when nearly full size
    float rate = (p.radius / p.threshold);
    return rate >= 0.95;
}

uint divide_particle(uint idx, float2 offset)
{
    Particle parent = _Particles[idx];
    Particle child = create();

    // Halve the size
    float rh = parent.radius * 0.5;
    parent.radius = child.radius = max(rh, 0.1);

    // Offset positions
    float2 center = parent.position;
    parent.position = center - offset;
    child.position = center + offset;

    // Random threshold for child
    child.threshold = rh * lerp(1.25, 2.0, nrand(float2(_Time, idx)));

    // Get child index from pool and store
    uint cidx = _ParticlePoolConsume.Consume();
    _Particles[cidx] = child;
    _Particles[idx] = parent;

    return cidx;
}
```

## Network Structure with Edges

### Edge Structure

```csharp
[StructLayout(LayoutKind.Sequential)]
public struct Edge_t
{
    public int a, b;        // Connected particle indices
    public Vector2 force;   // Spring force
    uint alive;             // Active flag
}
```

### Connecting Particles with Edges

```hlsl
void connect(int a, int b)
{
    uint eidx = _EdgePoolConsume.Consume();

    // Atomic increment of link counts
    InterlockedAdd(_Particles[a].links, 1);
    InterlockedAdd(_Particles[b].links, 1);

    Edge e;
    e.a = a;
    e.b = b;
    e.force = float2(0, 0);
    e.alive = true;
    _Edges[eidx] = e;
}
```

### Atomic Operations

**InterlockedAdd** prevents race conditions when multiple threads modify the same memory location. It guarantees that read-modify-write operations complete atomically without interference from other threads.

```hlsl
// Safe increment across threads
InterlockedAdd(_Particles[a].links, 1);
```

### Closed Network Division Pattern

Creates triangular networks:

```hlsl
void divide_edge_closed(uint idx)
{
    Edge e = _Edges[idx];
    Particle pa = _Particles[e.a];
    Particle pb = _Particles[e.b];

    if ((pa.links == 1) || (pb.links == 1))
    {
        // Create triangle: divide and connect to both endpoints
        uint cidx = divide_particle(e.a);
        connect(e.a, cidx);
        connect(cidx, e.b);
    }
    else
    {
        // Insert child between parent and neighbor
        float2 dir = pb.position - pa.position;
        float2 offset = normalize(dir) * pa.radius * 0.25;
        uint cidx = divide_particle(e.a, offset);

        connect(e.a, cidx);

        // Convert existing edge to connect child
        InterlockedAdd(_Particles[e.a].links, -1);
        InterlockedAdd(_Particles[cidx].links, 1);
        e.a = cidx;
    }

    _Edges[idx] = e;
}
```

### Spring Forces Between Connected Particles

```hlsl
THREAD
void UpdateEdges(uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    Edge e = _Edges[idx];
    e.force = float2(0, 0);

    if (!e.alive) { _Edges[idx] = e; return; }

    Particle pa = _Particles[e.a];
    Particle pb = _Particles[e.b];

    if (!pa.alive || !pb.alive) { _Edges[idx] = e; return; }

    // Calculate spring force
    float2 dir = pa.position - pb.position;
    float r = pa.radius + pb.radius;
    float len = length(dir);

    if (abs(len - r) > 0)
    {
        float l = ((len - r) / r);
        e.force = normalize(dir) * l * _Spring;
    }

    _Edges[idx] = e;
}

THREAD
void SpringEdges(uint3 id : SV_DispatchThreadID)
{
    Particle p = _Particles[id.x];
    if (!p.alive || p.links <= 0) return;

    // More connections = weaker individual force
    float dif = 1.0 / p.links;

    // Find all connected edges and apply forces
    uint count, strides;
    _Edges.GetDimensions(count, strides);

    for (uint i = 0; i < count; i++)
    {
        Edge e = _Edges[i];
        if (!e.alive) continue;

        if (e.a == (int)id.x)
            p.velocity -= e.force * dif;
        else if (e.b == (int)id.x)
            p.velocity += e.force * dif;
    }

    _Particles[id.x] = p;
}
```

### Branch Division Pattern

Creates tree-like structures:

```hlsl
void divide_edge_branch(uint idx)
{
    Edge e = _Edges[idx];
    Particle pa = _Particles[e.a];
    Particle pb = _Particles[e.b];

    // Divide from particle with fewer connections
    uint i = lerp(e.b, e.a, step(pa.links, pb.links));

    uint cidx = divide_particle(i);
    connect(i, cidx);
}
```

Adjusting `_MaxLink` (maximum connections per particle) dramatically changes branching behavior.

## Key Takeaways

1. **Append/ConsumeStructuredBuffer** enables dynamic GPU object management
2. **Object pooling** on GPU uses index-based activation/deactivation
3. **Ping-pong buffers** prevent data races in parallel particle updates
4. **Atomic operations** (InterlockedAdd) ensure thread-safe counter updates
5. **Network structures** emerge from edge-connected particles
6. **Division patterns** create different growth behaviors (closed networks vs. branching)
7. The technique can extend to **3D** for organic mesh generation

## Further Resources

- 3D extension: https://github.com/mattatz/CellularGrowth
- Andy Lomas: Morphogenetic Creations
- J.A. Kaandorp: Computational Biology
- Max Cooper music videos (using Houdini)

## References

- iGeo Tutorial: http://igeo.jp/tutorial/55.html
- HLSL Atomic Functions: https://docs.microsoft.com/windows/desktop/direct3d11/direct3d-11-advanced-stages-cs-atomic-functions
