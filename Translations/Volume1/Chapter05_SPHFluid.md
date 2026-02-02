# Chapter 5: SPH Particle-Based Fluid Simulation

*Original author: @takao*
*Translation and annotations by Claude*

---

## Introduction

The previous chapter covered grid-based fluid simulation. This chapter explores the other major approach to fluid simulation: **particle methods**, specifically **SPH (Smoothed Particle Hydrodynamics)**.

Some explanations are simplified for clarity—please understand if certain expressions are incomplete.

---

## Background Knowledge

### Eulerian vs Lagrangian Perspectives

There are two fundamental ways to observe fluid motion:

**Eulerian Perspective**: Fix observation points in space at regular intervals and analyze fluid motion at those fixed locations.

**Lagrangian Perspective**: Float observation points that move with the flow and analyze fluid motion from those moving viewpoints.

Generally:
- **Grid-based methods** = Eulerian perspective
- **Particle methods** = Lagrangian perspective

> **Why This Matters**
>
> The Eulerian view is like standing on a bridge watching water flow past. The Lagrangian view is like riding a leaf floating downstream. Each perspective leads to different mathematical formulations and implementation approaches.

### The Lagrangian Derivative (Material Derivative)

The mathematical treatment differs between these perspectives. In the Eulerian view, a physical quantity (like velocity or density) at position $\vec{x}$ and time $t$ is written as:

$$\phi = \phi(\vec{x}, t)$$

Its time derivative is simply:

$$\frac{\partial \phi}{\partial t}$$

This represents the rate of change at a **fixed** position—the Eulerian derivative.

In the Lagrangian view, observation points move with the flow (**advection**). A point initially at $\vec{x}_0$ is at time $t$ located at:

$$\vec{x}(\vec{x}_0, t)$$

So the physical quantity becomes:

$$\phi = \phi(\vec{x}(\vec{x}_0, t), t)$$

Following the definition of derivatives and using the chain rule:

$$\lim_{\Delta t \to 0} \frac{\phi(\vec{x}(\vec{x}_0, t + \Delta t), t + \Delta t) - \phi(\vec{x}(\vec{x}_0, t), t)}{\Delta t}$$

$$= \sum_i \frac{\partial \phi}{\partial x_i} \frac{\partial x_i}{\partial t} + \frac{\partial \phi}{\partial t}$$

$$= \left(\frac{\partial}{\partial t} + \vec{u} \cdot \text{grad}\right) \phi$$

To simplify notation, we introduce the **material derivative operator**:

$$\frac{D}{Dt} := \frac{\partial}{\partial t} + \vec{u} \cdot \text{grad}$$

> **Key Insight**
>
> The material derivative has two parts:
> - $\frac{\partial}{\partial t}$: Change at a fixed point (local change)
> - $\vec{u} \cdot \text{grad}$: Change due to moving to a new location (convective change)
>
> In particle methods, since particles naturally move with the flow, the material derivative becomes much simpler to compute!

### The Incompressibility Condition

When fluid velocity is much slower than the speed of sound, we can assume no volume change occurs. This **incompressibility condition** is expressed as:

$$\nabla \cdot \vec{u} = 0$$

This means there are no sources or sinks within the fluid—what flows in must flow out.

---

## Particle Method Simulation

Particle methods divide fluid into small **particles** and observe motion from the Lagrangian perspective. These particles are our moving observation points.

Many particle methods exist:

- **SPH** (Smoothed Particle Hydrodynamics)
- **FLIP** (Fluid Implicit Particle)
- **PIC** (Particle In Cell)
- **MPS** (Moving Particle Semi-implicit)
- **MPM** (Material Point Method)

### Deriving Navier-Stokes for Particle Methods

The particle-method form of the Navier-Stokes equation is:

$$\frac{D\vec{u}}{Dt} = -\frac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \vec{u} + \vec{g}$$

Notice something different from the grid-based version? The **advection term is gone**!

> **Why No Advection Term?**
>
> Remember the relationship between Eulerian and Lagrangian derivatives. In particle methods, observation points move with the flow, so advection is handled automatically by moving the particles. We don't need to calculate it separately in the NS equation!

Real fluids are collections of molecules—essentially a particle system. But simulating actual molecules is computationally impossible, so we use larger "blobs" representing fluid regions.

Each particle has:
- Mass $m$
- Position $\vec{x}$
- Velocity $\vec{u}$
- Volume $V$

For each particle, we calculate external forces $\vec{f}$, solve Newton's equation $m\vec{a} = \vec{f}$ to get acceleration, and determine movement for the next timestep.

### Force 1: Pressure

Fluid always flows from high pressure to low pressure regions. Taking the gradient of the pressure scalar field gives us the direction of maximum pressure increase. Since force acts from high to low pressure, we negate it: $-\nabla p$.

Since particles have volume, the pressure force on a particle is $-V\nabla p$.

### Force 2: Viscosity

Viscous fluids (honey, melted chocolate) resist deformation. In particle terms: **a particle's velocity tends toward the average of its neighbors' velocities**.

The Laplacian operator computes this neighborhood averaging, giving us: $\mu \nabla \cdot \nabla \vec{u}$

where $\mu$ is the **dynamic viscosity coefficient**.

### Combining Forces

Putting pressure, viscosity, and external forces (gravity) into Newton's equation:

$$m \frac{D\vec{u}}{Dt} = -V\nabla p + V\mu \nabla \cdot \nabla \vec{u} + m\vec{g}$$

Since $m = \rho V$:

$$\rho \frac{D\vec{u}}{Dt} = -\nabla p + \mu \nabla \cdot \nabla \vec{u} + \rho\vec{g}$$

Dividing by $\rho$ and substituting $\nu = \frac{\mu}{\rho}$ (kinematic viscosity):

$$\frac{D\vec{u}}{Dt} = -\frac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \vec{u} + \vec{g}$$

This is our particle-method Navier-Stokes equation!

### Advection in Particle Methods

Since particles ARE the observation points, advection is simply moving the particles. Using finite differences with small timestep $\Delta t$:

**Velocity update:**
$$\vec{u}_{n+1} = \vec{u}_n + \Delta t \cdot \vec{a}$$

**Position update:**
$$\vec{x}_{n+1} = \vec{x}_n + \Delta t \cdot \vec{u}$$

This is called the **Forward Euler method**.

---

## SPH Fluid Simulation

We can't solve differential equations directly on computers—we need approximations. **SPH (Smoothed Particle Hydrodynamics)** provides this approximation.

SPH originated in astrophysics for simulating celestial body collisions. Desbrun et al. (1996) adapted it for CG fluid simulation. It parallelizes well and modern GPUs can handle massive particle counts in real-time.

### The Kernel Function (Weight Function)

In SPH, each particle has an **influence radius**. Nearby particles have stronger influence; distant particles have less.

This influence is described by a **kernel function** (or weight function) $W$. The kernel is shaped like a smooth bump centered on the particle, falling to zero at the influence radius $h$.

> **Intuition**
>
> Think of each particle as a soft blob of influence. When calculating properties at any point, you sum contributions from all nearby particles, weighted by how close they are. Closer = more influence.

### Discretizing Physical Quantities

Any physical quantity $\phi$ at position $\vec{x}$ can be approximated as:

$$\phi(\vec{x}) = \sum_{j \in N} m_j \frac{\phi_j}{\rho_j} W(\vec{x}_j - \vec{x}, h)$$

Where:
- $N$ = set of neighboring particles
- $m$ = particle mass
- $\rho$ = particle density
- $h$ = influence radius
- $W$ = kernel function

**Gradient:**
$$\nabla \phi(\vec{x}) = \sum_{j \in N} m_j \frac{\phi_j}{\rho_j} \nabla W(\vec{x}_j - \vec{x}, h)$$

**Laplacian:**
$$\nabla^2 \phi(\vec{x}) = \sum_{j \in N} m_j \frac{\phi_j}{\rho_j} \nabla^2 W(\vec{x}_j - \vec{x}, h)$$

> **Key Insight**
>
> Notice that differential operators (gradient, Laplacian) are applied only to the kernel function, not the physical quantities! This is the genius of SPH—we precompute kernel derivatives analytically.

Different kernel functions are used for different quantities (density, pressure, viscosity) for numerical stability.

### Density Calculation

$$\rho(\vec{x}) = \sum_{j \in N} m_j W_{\text{poly6}}(\vec{x}_j - \vec{x}, h)$$

Uses the **Poly6 kernel** for smooth density estimation.

### Viscosity Calculation

$$f_i^{\text{visc}} = \mu \sum_{j \in N} m_j \frac{\vec{u}_j - \vec{u}_i}{\rho_j} \nabla^2 W_{\text{visc}}(\vec{x}_j - \vec{x}, h)$$

Uses the **Viscosity kernel's Laplacian**.

### Pressure Calculation

$$f_i^{\text{press}} = -\frac{1}{\rho_i} \sum_{j \in N} m_j \frac{p_j + p_i}{2\rho_j} \nabla W_{\text{spiky}}(\vec{x}_j - \vec{x}, h)$$

Uses the **Spiky kernel's gradient** (better handles close particles).

Particle pressure is computed via the **Tait equation**:

$$p = B \left\{ \left(\frac{\rho}{\rho_0}\right)^\gamma - 1 \right\}$$

> **Why Tait Instead of Poisson?**
>
> True incompressibility requires solving a Poisson equation—expensive! The Tait equation approximates this much faster, though with some compressibility. This trade-off between accuracy and speed is why SPH is considered weaker at pressure than grid methods.

---

## SPH Implementation

Sample code is in Assets/**SPHFluid** in the repository. This implementation prioritizes simplicity over optimization or numerical stability.

### Parameters

```csharp
NumParticleEnum particleNum = NumParticleEnum.NUM_8K;  // Particle count
float smoothlen = 0.012f;              // Particle radius (influence radius h)
float pressureStiffness = 200.0f;      // Pressure coefficient
float restDensity = 1000.0f;           // Rest density
float particleMass = 0.0002f;          // Particle mass
float viscosity = 0.1f;                // Viscosity coefficient
float maxAllowableTimestep = 0.005f;   // Time step
float wallStiffness = 3000.0f;         // Wall repulsion (penalty method)
int iterations = 4;                    // Iterations per frame
Vector2 gravity = new Vector2(0.0f, -0.5f);  // Gravity
Vector2 range = new Vector2(1, 1);           // Simulation space
```

### Kernel Coefficient Pre-calculation

Since kernel coefficients don't change during simulation, compute them once on CPU:

```csharp
densityCoef = particleMass * 4f / (Mathf.PI * Mathf.Pow(smoothlen, 8));
gradPressureCoef = particleMass * -30.0f / (Mathf.PI * Mathf.Pow(smoothlen, 5));
lapViscosityCoef = particleMass * 20f / (3 * Mathf.PI * Mathf.Pow(smoothlen, 5));
```

### Density Kernel

```hlsl
[numthreads(THREAD_SIZE_X, 1, 1)]
void DensityCS(uint3 DTid : SV_DispatchThreadID) {
    uint P_ID = DTid.x;  // Current particle ID

    float h_sq = _Smoothlen * _Smoothlen;
    float2 P_position = _ParticlesBufferRead[P_ID].position;

    // Neighbor search (O(n^2) - simple but slow)
    float density = 0;
    for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
        if (N_ID == P_ID) continue;  // Skip self

        float2 N_position = _ParticlesBufferRead[N_ID].position;
        float2 diff = N_position - P_position;
        float r_sq = dot(diff, diff);

        // Only particles within influence radius
        if (r_sq < h_sq) {
            density += CalculateDensity(r_sq);
        }
    }

    _ParticlesDensityBufferWrite[P_ID].density = density;
}

inline float CalculateDensity(float r_sq) {
    const float h_sq = _Smoothlen * _Smoothlen;
    // Poly6 kernel (only uses squared distances - no sqrt needed!)
    return _DensityCoef * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
}
```

> **Performance Note**
>
> The O(n²) neighbor search is simple but slow. Production implementations use spatial hashing or grid-based neighbor finding to achieve O(n) complexity.

### Pressure Kernel

```hlsl
[numthreads(THREAD_SIZE_X, 1, 1)]
void PressureCS(uint3 DTid : SV_DispatchThreadID) {
    uint P_ID = DTid.x;

    float P_density = _ParticlesDensityBufferRead[P_ID].density;
    float P_pressure = CalculatePressure(P_density);

    _ParticlesPressureBufferWrite[P_ID].pressure = P_pressure;
}

// Tait equation for pressure
inline float CalculatePressure(float density) {
    return _PressureStiffness * max(pow(density / _RestDensity, 7) - 1, 0);
}
```

### Force Kernel (Pressure + Viscosity)

```hlsl
[numthreads(THREAD_SIZE_X, 1, 1)]
void ForceCS(uint3 DTid : SV_DispatchThreadID) {
    uint P_ID = DTid.x;

    float2 P_position = _ParticlesBufferRead[P_ID].position;
    float2 P_velocity = _ParticlesBufferRead[P_ID].velocity;
    float  P_density = _ParticlesDensityBufferRead[P_ID].density;
    float  P_pressure = _ParticlesPressureBufferRead[P_ID].pressure;

    const float h_sq = _Smoothlen * _Smoothlen;

    float2 press = float2(0, 0);
    float2 visco = float2(0, 0);

    for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
        if (N_ID == P_ID) continue;

        float2 N_position = _ParticlesBufferRead[N_ID].position;
        float2 diff = N_position - P_position;
        float r_sq = dot(diff, diff);

        if (r_sq < h_sq) {
            float N_density = _ParticlesDensityBufferRead[N_ID].density;
            float N_pressure = _ParticlesPressureBufferRead[N_ID].pressure;
            float2 N_velocity = _ParticlesBufferRead[N_ID].velocity;
            float r = sqrt(r_sq);

            // Pressure force (Spiky kernel gradient)
            press += CalculateGradPressure(...);

            // Viscosity force (Viscosity kernel Laplacian)
            visco += CalculateLapVelocity(...);
        }
    }

    float2 force = press + _Viscosity * visco;
    _ParticlesForceBufferWrite[P_ID].acceleration = force / P_density;
}
```

### Integration Kernel (Position Update)

```hlsl
[numthreads(THREAD_SIZE_X, 1, 1)]
void IntegrateCS(uint3 DTid : SV_DispatchThreadID) {
    const unsigned int P_ID = DTid.x;

    float2 position = _ParticlesBufferRead[P_ID].position;
    float2 velocity = _ParticlesBufferRead[P_ID].velocity;
    float2 acceleration = _ParticlesForceBufferRead[P_ID].acceleration;

    // Mouse interaction
    if (distance(position, _MousePos.xy) < _MouseRadius && _MouseDown) {
        float2 dir = position - _MousePos.xy;
        float pushBack = _MouseRadius - length(dir);
        acceleration += 100 * pushBack * normalize(dir);
    }

    // Wall boundaries (penalty method)
    float dist = dot(float3(position, 1), float3(1, 0, 0));
    acceleration += min(dist, 0) * -_WallStiffness * float2(1, 0);
    // ... (other walls)

    // Add gravity
    acceleration += _Gravity;

    // Forward Euler integration
    velocity += _TimeStep * acceleration;
    position += _TimeStep * velocity;

    _ParticlesBufferWrite[P_ID].position = position;
    _ParticlesBufferWrite[P_ID].velocity = velocity;
}
```

> **Penalty Method for Walls**
>
> When a particle penetrates a wall boundary, push it back with force proportional to penetration depth. Simple and effective for basic boundary handling.

### Main Simulation Loop

```csharp
private void RunFluidSolver() {
    int kernelID = -1;
    int threadGroupsX = numParticles / THREAD_SIZE_X;

    // Step 1: Density
    kernelID = fluidCS.FindKernel("DensityCS");
    fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
    fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferWrite", ...);
    fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

    // Step 2: Pressure (from density)
    kernelID = fluidCS.FindKernel("PressureCS");
    // ...

    // Step 3: Forces (pressure + viscosity)
    kernelID = fluidCS.FindKernel("ForceCS");
    // ...

    // Step 4: Integrate (update positions)
    kernelID = fluidCS.FindKernel("IntegrateCS");
    // ...

    // Swap read/write buffers
    SwapComputeBuffer(ref particlesBufferRead, ref particlesBufferWrite);
}
```

### Sub-stepping for Stability

Smaller timesteps = more accurate simulation. But $\Delta t = 1/60$ (for 60 FPS) is too large—particles explode!

Solution: Run multiple iterations per frame with smaller timesteps:

```csharp
// Run multiple iterations with smaller timestep for stability
for (int i = 0; i < iterations; i++) {
    RunFluidSolver();
}
```

With `iterations = 4` and 60 FPS, effective timestep is $\Delta t = 1/(60 \times 4) = 1/240$.

### Double Buffering

Since particles interact with each other, we can't let data change mid-calculation. Use two buffers and swap each frame:

```csharp
void SwapComputeBuffer(ref ComputeBuffer ping, ref ComputeBuffer pong) {
    ComputeBuffer temp = ping;
    ping = pong;
    pong = temp;
}
```

### Particle Rendering

Particles are rendered as point sprites (billboards) using geometry shaders:

```csharp
void DrawParticle() {
    RenderParticleMat.SetPass(0);
    RenderParticleMat.SetColor("_WaterColor", WaterColor);
    RenderParticleMat.SetBuffer("_ParticlesBuffer", solver.ParticlesBufferRead);
    Graphics.DrawProceduralNow(MeshTopology.Points, solver.NumParticles);
}
```

The vertex shader reads particle positions from the buffer using `SV_VertexID`, and the geometry shader expands each point into a camera-facing quad (billboard).

---

## Results

The simulation produces interactive 2D fluid that responds to mouse input.

Video: https://youtu.be/KJVu26zeK2w

---

## Summary

This chapter demonstrated SPH fluid simulation. SPH treats fluid as a generic particle system, making it intuitive and parallelizable.

Beyond SPH, many other fluid simulation methods exist. We hope this chapter sparks interest in exploring other physics simulations and expanding your creative toolkit.

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **Lagrangian = moving observation points** → Particles naturally handle advection
>
> 2. **Material derivative** $\frac{D}{Dt}$ → Combines local and convective change
>
> 3. **SPH kernel functions** → Weight contributions by distance, apply derivatives to kernels only
>
> 4. **Different kernels for different quantities** → Poly6 (density), Spiky (pressure), Viscosity (viscosity)
>
> 5. **Tait equation** → Fast pressure approximation (trades accuracy for speed)
>
> 6. **Sub-stepping** → Multiple small timesteps per frame for stability
>
> 7. **Double buffering** → Prevents read/write conflicts in parallel computation

---

## SPH vs Grid Methods Comparison

| Aspect | Grid (Stable Fluids) | Particle (SPH) |
|--------|---------------------|----------------|
| **Perspective** | Eulerian (fixed points) | Lagrangian (moving points) |
| **Advection** | Explicit calculation needed | Automatic (particles move) |
| **Pressure** | Accurate (Poisson solver) | Approximate (Tait equation) |
| **Neighbor finding** | Trivial (grid structure) | Requires spatial data structure |
| **Free surfaces** | Difficult | Natural |
| **Splashing** | Difficult | Natural |
| **Memory** | Fixed (grid resolution) | Scales with particle count |

---

## References

- Desbrun and Cani, "Smoothed Particles: A new paradigm for animating highly deformable bodies," 1996
- "Fluid Simulation for Computer Graphics" - Robert Bridson

---

*Next chapter: Geometry Shaders for Grass Rendering!*
