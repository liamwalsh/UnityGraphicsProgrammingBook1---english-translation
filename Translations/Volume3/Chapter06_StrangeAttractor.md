# Chapter 6: Strange Attractor

**Author: Sakota**

## Introduction

This chapter visualizes **Strange Attractors** - phenomena where states governed by specific differential or difference equations exhibit nonlinear chaotic behavior - using Unity and GPU computation.

**Sample Project**: `StrangeAttractors` in the Unity Graphics Programming 3 repository.

### Requirements

- ComputeShader support (Shader Model 5.0)
- Tested on Unity 2018.2.9f1

## What is a Strange Attractor?

In dissipative systems (non-equilibrium systems with energy input and output), states that maintain stable orbits over time are called **Attractors**.

Among these, attractors where small differences in initial conditions amplify over time, exhibiting chaotic behavior, are called **Strange Attractors**.

This chapter covers two examples:
1. **Lorenz Attractor**
2. **Thomas' Cyclically Symmetric Attractor**

## Lorenz Attractor

### The Butterfly Effect

The term "butterfly effect" originated from meteorologist Edward N. Lorenz's 1972 lecture titled "Does the Flap of a Butterfly's Wings in Brazil Set Off a Tornado in Texas?"

This phrase describes how tiny initial differences don't necessarily lead to similar results mathematically - they can amplify chaotically into unpredictable behavior.

The mathematical properties behind this observation were published by Lorenz in 1963 as the **Lorenz Attractor**.

### Lorenz Equations

The Lorenz system is described by these nonlinear ordinary differential equations:

$$\frac{dx}{dt} = -px + py$$

$$\frac{dy}{dt} = -xz + rx - y$$

$$\frac{dz}{dt} = xy - bz$$

When parameters are set to **p=10, r=28, b=8/3**, the system exhibits chaotic Strange Attractor behavior.

### Implementation

#### Particle Structure

```csharp
protected struct Params
{
    Vector3 emitPos;
    Vector3 position;
    Vector3 velocity;
    float   life;
    Vector2 size;     // x = current, y = target
    Vector4 color;

    public Params(Vector3 emitPos, float size, Color color)
    {
        this.emitPos = emitPos;
        this.position = Vector3.zero;
        this.velocity = Vector3.zero;
        this.life = 0;
        this.size = new Vector2(0, size);
        this.color = color;
    }
}
```

This structure is defined in the abstract `StrangeAttractor.cs` class for reuse across different attractors.

#### Buffer Initialization

```csharp
protected sealed override void InitializeComputeBuffer()
{
    if (cBuffer != null) cBuffer.Release();

    cBuffer = new ComputeBuffer(instanceCount, Marshal.SizeOf(typeof(Params)));
    Params[] parameters = new Params[cBuffer.count];

    for (int i = 0; i < instanceCount; i++)
    {
        var normalize = (float)i / instanceCount;
        var color = gradient.Evaluate(normalize);
        parameters[i] = new Params(
            Random.insideUnitSphere * emitterSize * normalize,
            particleSize,
            color
        );
    }
    cBuffer.SetData(parameters);
}
```

Particles are colored by ID using a gradient, creating beautiful trails as particles follow the attractor.

#### Parameter Setup

```csharp
[SerializeField, Tooltip("Default is 10")]
float p = 10f;
[SerializeField, Tooltip("Default is 28")]
float r = 28f;
[SerializeField, Tooltip("Default is 8/3")]
float b = 2.666667f;

protected override void UpdateShaderUniforms()
{
    computeShaderInstance.SetFloat(pId, p);
    computeShaderInstance.SetFloat(rId, r);
    computeShaderInstance.SetFloat(bId, b);
}
```

#### Emit Kernel

```hlsl
#pragma kernel Emit
#pragma kernel Iterator

#define THREAD_X 128
#define THREAD_Y 1
#define THREAD_Z 1
#define DT 0.022

struct Params
{
    float3 emitPos;
    float3 position;
    float3 velocity;
    float  life;
    float2 size;
    float4 color;
};

RWStructuredBuffer<Params> buf;

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void Emit(uint id : SV_DispatchThreadID)
{
    Params p = buf[id];
    p.life = (float)id * -1e-05;  // Slight delay per particle
    p.position = p.emitPos;
    p.size.x = 0.0;  // Start invisible
    buf[id] = p;
}
```

The negative `life` value creates a staggered emission effect - particles don't all start at once, which:
- Prevents all particles from following identical paths
- Creates beautiful gradient trails as particles spread out

#### Iterator Kernel (Lorenz Equations)

```hlsl
#define DT 0.022

float p;
float r;
float b;

float3 LorenzAttractor(float3 pos)
{
    float dxdt = (p * (pos.y - pos.x));
    float dydt = (pos.x * (r - pos.z) - pos.y);
    float dzdt = (pos.x * pos.y - b * pos.z);
    return float3(dxdt, dydt, dzdt) * DT;
}

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void Iterator(uint id : SV_DispatchThreadID)
{
    Params p = buf[id];
    p.life.x += DT;

    // Size scales with velocity for natural appearance
    p.size.x = p.size.y * saturate(length(p.velocity));

    if (p.life.x > 0)
    {
        p.velocity = LorenzAttractor(p.position);
        p.position += p.velocity;
    }
    buf[id] = p;
}
```

### Why Use Fixed Delta Time?

The code uses a constant `DT = 0.022` instead of Unity's `Time.deltaTime`. This is critical for two reasons:

1. **Frame rate independence**: When frame rate drops, `Time.deltaTime` becomes large, causing coarse, inaccurate calculations that distort the attractor shape.

2. **Numerical stability**: Strange Attractors are sensitive to time step size. Wrong values can cause the system to either collapse to a point or diverge to infinity.

## Thomas' Cyclically Symmetric Attractor

Published by biologist Rene Thomas, this attractor is notable for:
- Stable behavior regardless of initial conditions
- Unique, visually interesting shape

### Thomas' Equations

$$\frac{dx}{dt} = \sin y - bx$$

$$\frac{dy}{dt} = \sin z - by$$

$$\frac{dz}{dt} = \sin x - bz$$

When **b ≈ 0.208186**: Chaotic Strange Attractor behavior
When **b ≈ 0**: Particles drift through space

### Implementation

```csharp
protected sealed override void InitializeComputeBuffer()
{
    if (cBuffer != null) cBuffer.Release();

    cBuffer = new ComputeBuffer(instanceCount, Marshal.SizeOf(typeof(Params)));
    Params[] parameters = new Params[cBuffer.count];

    for (int i = 0; i < instanceCount; i++)
    {
        var normalize = (float)i / instanceCount;
        var color = gradient.Evaluate(normalize);
        parameters[i] = new Params(
            Random.insideUnitSphere * emitterSize * normalize,
            particleSize,
            color
        );
    }
    cBuffer.SetData(parameters);
}
```

For visual appeal, particles are colored with a gradient from center to edge, creating a mantle-like appearance in the spherical initial distribution.

#### Thomas Attractor Kernel

```hlsl
float b;

float3 ThomasAttractor(float3 pos)
{
    float dxdt = -b * pos.x + sin(pos.y);
    float dydt = -b * pos.y + sin(pos.z);
    float dzdt = -b * pos.z + sin(pos.x);
    return float3(dxdt, dydt, dzdt) * DT;
}

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void Emit(uint id : SV_DispatchThreadID)
{
    Params p = buf[id];
    p.life = (float)id * -1e-05;
    p.position = p.emitPos;
    p.size.x = p.size.y;  // Start visible (unlike Lorenz)
    buf[id] = p;
}

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void Iterator(uint id : SV_DispatchThreadID)
{
    Params p = buf[id];
    p.life.x += DT;

    if (p.life.x > 0)
    {
        p.velocity = ThomasAttractor(p.position);
        p.position += p.velocity;
    }
    buf[id] = p;
}
```

Unlike the Lorenz implementation, particles start visible (`p.size.x = p.size.y`) because we want to see the initial spherical distribution transform into the attractor shape.

## Other Strange Attractors

There are many other fascinating Strange Attractors to explore:

- **Ueda Attractor**: 2D motion (Kyoto University)
- **Aizawa Attractor**: Spinning motion
- **Rossler Attractor**: Spiral bands
- **Chen Attractor**: Double scroll
- **Halvorsen Attractor**: Three interlocked loops

Each has unique visual characteristics and mathematical properties.

## Key Takeaways

1. **Strange Attractors** are chaotic systems that maintain stable orbits despite sensitivity to initial conditions
2. **Lorenz Attractor** (p=10, r=28, b=8/3) demonstrates the butterfly effect
3. **Thomas' Attractor** (b≈0.208186) creates unique symmetric patterns
4. **Fixed delta time** is essential for stable, consistent attractor shapes
5. **Staggered particle emission** creates beautiful gradient trails
6. **GPU implementation** enables real-time visualization of millions of particles
7. Strange Attractors offer **low computational cost** with **high visual impact**

## Visual Techniques

- **Gradient coloring** by particle ID creates depth
- **Velocity-based sizing** makes particles appear to accelerate
- **Spherical initial distribution** shows dramatic transformation
- **Delayed emission** prevents uniform paths

## References

- http://paulbourke.net/fractals/lorenz/
- https://en.wikipedia.org/wiki/Thomas%27_cyclically_symmetric_attractor
- Lorenz, E. N.: Deterministic Nonperiodic Flow, Journal of Atmospheric Sciences, Vol.20, pp.130-141, 1963
- Thomas, Rene (1999): "Deterministic chaos seen in terms of feedback circuits"
- http://www.algosome.com/articles/aizawa-attractor-chaos.html
