# Chapter 3: GPU-Based Cloth Simulation

**Author:** Oishi
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** GPUClothSimulation

---

## Introduction

Simulating deformable planar objects like flags and clothing under external forces is called **Cloth Simulation**. This is essential for animation in CG, and extensive research exists in this field.

While Unity already has built-in cloth simulation, this chapter introduces simple cloth simulation theory and GPU implementation for the purposes of learning parallel computation and understanding simulation properties and parameters.

---

## Algorithm Overview

### Mass-Spring System

Objects like springs, rubber, and cushions that deform under force and return to original shape are called **elastic bodies**. Since elastic bodies cannot be represented by a single position or orientation, we represent them as points connected by links:

- **Mass points** - Dimensionless points with mass
- **Springs** - Connections between mass points with elastic properties

Simulating elastic bodies by calculating spring stretching/compression is called a **Mass-Spring System**. When applied to a 2D array of mass points for simulating flags and clothing, it's called **Cloth Simulation**.

### Spring Force

Each spring applies force to connected mass points according to:

$$F_{spring} = -k(l - l_0) - bv$$

Where:
- **l** = Current spring length (distance between connected mass points)
- **l_0** = Natural length (rest length with no load)
- **k** = Spring stiffness constant
- **v** = Mass point velocity
- **b** = Damping constant

This equation means springs always try to return to their natural length. Greater deviation from natural length produces greater force, with velocity-proportional damping.

### Spring Structure

The simulation uses two types of springs:

1. **Structure Springs** - Connect horizontally and vertically adjacent points
2. **Shear Springs** - Connect diagonally adjacent points (prevent extreme diagonal deformation)

Each mass point connects to up to 12 neighboring points:
- 8 immediate neighbors (structure + shear)
- 4 second-ring neighbors for additional stability

### Verlet Integration

This simulation uses **Verlet integration** - a numerical method commonly used in real-time applications and molecular dynamics.

Unlike typical velocity-based position calculation, Verlet integration computes the **next position** from the **current position** and **previous position**.

#### Derivation

Starting from Newton's equation of motion:

$$m\frac{d^2x(t)}{dt^2} = F$$

Using Taylor expansions:

$$x(t + \Delta t) = x(t) + \Delta t\frac{dx(t)}{dt} + \frac{1}{2}\Delta t^2\frac{d^2x(t)}{dt^2} + ...$$

$$x(t - \Delta t) = x(t) - \Delta t\frac{dx(t)}{dt} + \frac{1}{2}\Delta t^2\frac{d^2x(t)}{dt^2} - ...$$

Adding these equations and solving for the next position:

$$x(t + \Delta t) = 2x(t) - x(t - \Delta t) + \frac{\Delta t^2}{m}F(t)$$

Velocity is approximated as:

$$v(t) = \frac{x(t) - x(t - \Delta t)}{\Delta t}$$

This velocity approximation isn't highly accurate but suffices for spring damping calculations.

### Sphere Collision

Collision handling has two phases: **detection** and **response**.

#### Detection

$$\|x(t + \Delta t) - c\| - r < 0$$

Where c is the sphere center and r is the radius.

#### Response

If collision is detected, move the mass point to the sphere surface:

$$d = \frac{x(t + \Delta t) - c}{\|x(t + \Delta t) - c\|}$$

$$x'(t + \Delta t) = c + dr$$

The direction d approximates the surface normal at the collision point.

---

## Implementation

### Architecture Overview

```
GPUClothSimulation.cs
    ├── Creates/manages RenderTextures (position, previous position, normal)
    ├── Calls Kernels.compute kernels
    └── Handles simulation loop

Kernels.compute
    ├── CSInit - Initialize buffers
    └── CSSimulation - Main simulation step

GPUClothRenderer.cs
    └── Generates mesh, applies shader

ClothSurface.shader
    └── Deforms mesh using simulation data
```

### GPUClothSimulation.cs

```csharp
public class GPUClothSimulation : MonoBehaviour
{
    [Header("Simulation Parameters")]
    public float TimeStep = 0.01f;
    [Range(1, 16)]
    public int VerletIterationNum = 4;
    public Vector2Int ClothResolution = new Vector2Int(128, 128);
    public float RestLength = 0.02f;
    public float Stiffness = 10000.0f;
    public float Damp = 0.996f;
    public float Mass = 1.0f;
    public Vector3 Gravity = new Vector3(0.0f, -9.81f, 0.0f);

    [Header("References")]
    public Transform CollisionSphereTransform;
    public ComputeShader KernelCS;

    // Simulation buffers
    private RenderTexture[] _posBuff;      // Position (ping-pong)
    private RenderTexture[] _posPrevBuff;  // Previous position (ping-pong)
    private RenderTexture _normBuff;       // Normals

    const int numThreadsXY = 32;

    void Start()
    {
        var format = RenderTextureFormat.ARGBFloat;  // 32-bit per channel
        var filter = FilterMode.Point;  // No interpolation

        CreateRenderTexture(ref _posBuff, w, h, format, filter);
        CreateRenderTexture(ref _posPrevBuff, w, h, format, filter);
        CreateRenderTexture(ref _normBuff, w, h, format, filter);

        ResetBuffer();
        IsInit = true;
    }

    void Simulation()
    {
        float timestep = TimeStep / VerletIterationNum;

        // Set parameters
        cs.SetVector("_Gravity", Gravity);
        cs.SetFloat("_Stiffness", Stiffness);
        cs.SetFloat("_Damp", Damp);
        cs.SetFloat("_InverseMass", 1.0f / Mass);
        cs.SetFloat("_TimeStep", timestep);

        // Collision sphere parameters
        if (CollisionSphereTransform != null)
        {
            Vector3 pos = CollisionSphereTransform.position;
            float rad = CollisionSphereTransform.localScale.x * 0.5f + 0.01f;
            cs.SetBool("_EnableCollideSphere", true);
            cs.SetFloats("_CollideSphereParams",
                new float[] { pos.x, pos.y, pos.z, rad });
        }

        // Multiple iterations for stability
        for (var i = 0; i < VerletIterationNum; i++)
        {
            cs.SetTexture(kernelId, "_PositionBufferRO", _posBuff[0]);
            cs.SetTexture(kernelId, "_PositionPrevBufferRO", _posPrevBuff[0]);
            cs.SetTexture(kernelId, "_PositionBufferRW", _posBuff[1]);
            cs.SetTexture(kernelId, "_PositionPrevBufferRW", _posPrevBuff[1]);
            cs.SetTexture(kernelId, "_NormalBufferRW", _normBuff);

            cs.Dispatch(kernelId, groupThreadsX, groupThreadsY, 1);

            // Ping-pong buffer swap
            SwapBuffer(ref _posBuff[0], ref _posBuff[1]);
            SwapBuffer(ref _posPrevBuff[0], ref _posPrevBuff[1]);
        }
    }
}
```

### Kernels.compute

```hlsl
#pragma kernel CSInit
#pragma kernel CSSimulation

#define NUM_THREADS_XY 32

// Buffers
Texture2D<float4> _PositionPrevBufferRO;
Texture2D<float4> _PositionBufferRO;
RWTexture2D<float4> _PositionPrevBufferRW;
RWTexture2D<float4> _PositionBufferRW;
RWTexture2D<float4> _NormalBufferRW;

// Parameters
int2 _ClothResolution;
float2 _TotalClothLength;
float _RestLength;
float3 _Gravity;
float _Stiffness;
float _Damp;
float _InverseMass;
float _TimeStep;
bool _EnableCollideSphere;
float4 _CollideSphereParams;  // xyz = position, w = radius

// Neighbor offsets (12 connections)
static const int2 m_Directions[12] =
{
    int2(-1, -1), int2( 0, -1), int2( 1, -1), int2( 1,  0),
    int2( 1,  1), int2( 0,  1), int2(-1,  1), int2(-1,  0),
    int2(-2, -2), int2( 2, -2), int2( 2,  2), int2(-2,  2)
};

// Initialization kernel
[numthreads(NUM_THREADS_XY, NUM_THREADS_XY, 1)]
void CSInit(uint3 DTid : SV_DispatchThreadID)
{
    uint2 idx = DTid.xy;

    // Position: grid layout on X-Y plane
    float3 pos = float3(idx.x * _RestLength, idx.y * _RestLength, 0);
    pos.xy -= _TotalClothLength.xy * 0.5;

    // Normal: facing -Z
    float3 nrm = float3(0, 0, -1);

    _PositionPrevBufferRW[idx] = float4(pos, 1.0);
    _PositionBufferRW[idx] = float4(pos, 1.0);
    _NormalBufferRW[idx] = float4(nrm, 1.0);
}

// Simulation kernel
[numthreads(NUM_THREADS_XY, NUM_THREADS_XY, 1)]
void CSSimulation(uint2 DTid : SV_DispatchThreadID)
{
    int2 idx = (int2)DTid.xy;
    int2 res = _ClothResolution.xy;

    // Read current and previous positions
    float3 pos = _PositionBufferRO[idx].xyz;
    float3 posPrev = _PositionPrevBufferRO[idx].xyz;

    // Calculate velocity from position difference
    float3 vel = (pos - posPrev) / _TimeStep;

    float3 normal = (float3)0;
    float3 lastDiff = (float3)0;
    float iters = 0.0;

    // Initialize force with gravity
    float3 force = _Gravity.xyz;
    float invMass = _InverseMass;

    // Fix top edge (no simulation)
    if (idx.y == _ClothResolution.y - 1)
        return;

    // Process all 12 neighbors
    [unroll]
    for (int k = 0; k < 12; k++)
    {
        int2 neighCoord = m_Directions[k];

        // Skip if neighbor is outside bounds
        if (((idx.x + neighCoord.x) < 0) ||
            ((idx.x + neighCoord.x) > (res.x - 1)))
            continue;
        if (((idx.y + neighCoord.y) < 0) ||
            ((idx.y + neighCoord.y) > (res.y - 1)))
            continue;

        int2 idxNeigh = idx + neighCoord;
        float3 posNeigh = _PositionBufferRO[idxNeigh].xyz;
        float3 posDiff = posNeigh - pos;

        // Normal calculation using cross products
        float3 currDiff = normalize(posDiff);
        if ((iters > 0.0) && (k < 8))
        {
            float a = dot(currDiff, lastDiff);
            if (a > 0.0) {
                normal += cross(lastDiff, currDiff);
            }
        }
        lastDiff = currDiff;

        // Spring force calculation
        float restLength = length(neighCoord * _RestLength);
        force += (currDiff * (length(posDiff) - restLength)) * _Stiffness
               - vel * _Damp;

        if (k < 8) iters += 1.0;
    }

    // Finalize normal
    normal = normalize(normal / -(iters - 1.0));

    // Calculate acceleration (F = ma, a = F/m)
    float3 acc = force * invMass;

    // Verlet integration
    float3 tmp = pos;
    pos = pos * 2.0 - posPrev + acc * (_TimeStep * _TimeStep);
    posPrev = tmp;

    // Sphere collision
    if (_EnableCollideSphere)
    {
        float3 center = _CollideSphereParams.xyz;
        float radius = _CollideSphereParams.w;

        if (length(pos - center) < radius)
        {
            float3 collDir = normalize(pos - center);
            pos = center + collDir * radius;
        }
    }

    // Write results
    _PositionBufferRW[idx] = float4(pos, 1.0);
    _PositionPrevBufferRW[idx] = float4(posPrev, 1.0);
    _NormalBufferRW[idx] = float4(normal, 1.0);
}
```

---

## Parameters Guide

| Parameter | Description | Effect |
|-----------|-------------|--------|
| **TimeStep** | Simulation time per Update | Larger = faster motion, but may become unstable |
| **VerletIterationNum** | Kernel calls per frame | Larger = more stable, but higher compute cost |
| **ClothResolution** | Grid size (particles) | Larger = more detail, but may destabilize. Use multiples of 32. |
| **RestLength** | Spring natural length | Cloth size = Resolution x RestLength |
| **Stiffness** | Spring hardness | Larger = less stretching, but may destabilize |
| **Damp** | Velocity damping | Larger = less oscillation, slower movement |
| **Mass** | Particle mass | Larger = heavier, more dramatic movement |
| **Gravity** | Gravitational force | Combined with spring force for acceleration |

### Tips

- Press **R key** to reset simulation
- **EnableDebugOnGUI** shows position/normal textures on screen
- If simulation explodes, reset and adjust parameters

---

## Ping-Pong Buffering

The simulation uses **ping-pong buffering** - maintaining two sets of buffers:
1. Read from buffer A, write to buffer B
2. Swap buffers
3. Read from buffer B (now containing new data), write to buffer A
4. Repeat

This pattern is essential for GPU computation where reading and writing to the same buffer simultaneously causes race conditions.

---

## Key Takeaways

1. **Mass-Spring Systems** model elastic bodies using connected mass points
2. **Verlet Integration** calculates position from current and previous positions (no velocity storage)
3. **12-neighbor connectivity** provides structure (horizontal/vertical) and shear (diagonal) springs
4. **Ping-pong buffering** enables safe GPU read/write operations
5. **Multiple iterations per frame** improve stability with smaller effective timesteps
6. **Simple sphere collision** projects penetrating points to sphere surface

## Future Directions

This chapter covers basic cloth simulation. Advanced topics include:
- Complex geometry collision (not just spheres)
- Cloth self-collision
- Friction simulation
- Fiber structure modeling
- Large timestep stability improvements

## References

- Marco Fratarcangeli, "Game Engine Gems 2, GPGPU Cloth simulation using GLSL, OpenCL, and CUDA"
- Wikipedia - Verlet integration
- Makoto Fujisawa, "Fundamentals of Physical Simulation for CG"
- Koichi Sakai, "Physical Simulation with WebGL"
- Koichi Sakai, "Introduction to Dynamics Animation with OpenGL"
