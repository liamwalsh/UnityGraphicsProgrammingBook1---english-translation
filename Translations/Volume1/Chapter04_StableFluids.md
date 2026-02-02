# Chapter 4: Grid-Based Fluid Simulation

*Original author: @sakota*
*Translation and annotations by Claude*

---

## About This Chapter

This chapter explains grid-based fluid simulation using ComputeShaders, based on the famous "Stable Fluids" paper by Jos Stam.

---

## Sample Data

### Code

https://github.com/IndieVisualLab/UnityGraphicsProgramming

Located in the Assets/**StableFluids** folder.

### Runtime Requirements

- Environment supporting ComputeShaders with Shader Model 5.0
- Tested with Unity 5.6.2 and Unity 2017.1.1

---

## Introduction

This chapter explains grid-based fluid simulation and the mathematical concepts needed to implement it. But first, what exactly is "grid-based" simulation? To understand this, let's briefly explore how fluid mechanics analyzes "flow."

### Approaches in Fluid Mechanics

Fluid mechanics is characterized by mathematically formulating natural phenomena of "flow" to make them computationally tractable. But how can we quantify and analyze this "flow"?

Put simply, we can quantify it by deriving "the velocity at a moment in time." Mathematically speaking, this means analyzing the change in velocity vectors when taking time derivatives.

However, there are **two fundamental approaches** to analyzing flow:

**Approach 1: Eulerian Method**
Imagine water in a bathtub. Divide the water into a grid of fixed cells, and measure the velocity vector at each fixed grid location.

**Approach 2: Lagrangian Method**
Imagine dropping a rubber duck in the bathtub. Track and analyze the duck's movement itself.

> **Why This Matters**
>
> These two perspectives—fixed observation points (Eulerian) vs. moving with the flow (Lagrangian)—are fundamental to all fluid simulation. Understanding this distinction helps you choose the right approach for different effects.

### Types of Fluid Simulation

In computer graphics, fluid simulations can be broadly categorized into three types:

| Type | Example | Description |
|------|---------|-------------|
| **Grid-based** | Stable Fluids | Like the Eulerian approach—simulate velocity at fixed grid points |
| **Particle-based** | SPH (Smoothed Particle Hydrodynamics) | Like the Lagrangian approach—track individual particles |
| **Hybrid** | FLIP, PIC | Combines both approaches |

As you might guess from these names:
- **Grid-based methods** create a "field" of grid cells and simulate velocity at each cell (Eulerian style)
- **Particle-based methods** focus on individual particles and simulate their movement (Lagrangian style)

Each approach has strengths and weaknesses:

| Aspect | Grid-Based | Particle-Based |
|--------|-----------|----------------|
| Pressure calculation | Good | Challenging |
| Viscosity/Diffusion | Good | Challenging |
| Advection (transport) | Challenging | Good |

> **Why the Difference?**
>
> Grid methods work well for differential operators (pressure, diffusion) because neighboring cells are at known, fixed positions. But advection moves things between cells, which creates interpolation challenges.
>
> Particle methods naturally handle advection (particles just move!) but struggle with pressure because finding neighboring particles requires expensive spatial searches.

**Hybrid methods** like FLIP were developed to get the best of both worlds.

This chapter focuses on **Stable Fluids**, the seminal paper by Jos Stam presented at SIGGRAPH 1999, which describes grid-based simulation of incompressible viscous fluids.

---

## The Navier-Stokes Equations

Let's examine the Navier-Stokes equations for grid-based simulation:

### The Velocity Field Equation

$$\frac{\partial \vec{u}}{\partial t} = -(\vec{u} \cdot \nabla)\vec{u} + \nu \nabla^2 \vec{u} + \vec{f}$$

### The Density Field Equation

$$\frac{\partial \rho}{\partial t} = -(\vec{u} \cdot \nabla)\rho + \kappa \nabla^2 \rho + S$$

### The Continuity Equation (Mass Conservation)

$$\nabla \cdot \vec{u} = 0$$

The first equation describes the **velocity field**, the second describes the **density field**, and the third is the **continuity equation** (mass conservation law).

Let's unpack each of these.

> **Don't Panic!**
>
> These equations look intimidating, but we'll break them down piece by piece. Each term has a clear physical meaning, and the implementation follows naturally from understanding these meanings.

---

## The Continuity Equation (Mass Conservation)

Let's start with the simplest equation—the continuity equation—which serves as a constraint for simulating **incompressible** fluids.

When simulating fluids, we must clearly distinguish whether the target is **compressible** or **incompressible**:

- **Compressible fluids**: Density changes with pressure (e.g., gases)
- **Incompressible fluids**: Density is constant everywhere (e.g., water)

This chapter deals with incompressible fluids, so we must keep the **divergence** of the velocity field at zero in each cell. This means inflow and outflow must cancel out—if there's inflow, there must be equal outflow, causing velocity to propagate. This condition is expressed as:

$$\nabla \cdot \vec{u} = 0$$

This means "divergence equals zero." But what exactly is divergence?

### Divergence

$$\nabla \cdot \vec{u} = \nabla \cdot (u, v) = \frac{\partial u}{\partial x} + \frac{\partial v}{\partial y}$$

The **nabla operator** (∇) is a vector differential operator. For a 2D vector field, it represents the partial derivative notation $\left(\frac{\partial}{\partial x}, \frac{\partial}{\partial y}\right)$.

The nabla operator itself has no meaning alone—its operation changes depending on whether it's combined with a dot product (divergence), cross product (curl), or simply applied to a function (gradient).

> **Intuition: What Does Divergence Mean?**
>
> Divergence measures how much a vector field "spreads out" or "converges" at a point:
> - **Positive divergence**: Source (things spreading outward)
> - **Negative divergence**: Sink (things converging inward)
> - **Zero divergence**: What flows in must flow out
>
> For incompressible fluids, we enforce zero divergence everywhere—no sources or sinks allowed!

### Understanding Divergence Geometrically

To understand why the formula works, consider a single cell from the grid:

```
        ↑ v(x, y+Δy)
        |
    ----+----
   |         |
 → |    •    | →  u(x+Δx, y)
   |         |
    ---------
        |
        ↓ v(x, y)
```

Divergence calculates the net "outflow" from this cell. We can derive it by considering the flow across each boundary:

$$\frac{i(x + \Delta x, y)\Delta y - i(x,y)\Delta y + j(x, y + \Delta y)\Delta x - j(x,y)\Delta x}{\Delta x \Delta y}$$

$$= \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \frac{j(x, y+\Delta y) - j(x,y)}{\Delta y}$$

Taking the limit as Δ approaches zero:

$$\lim_{\Delta x \to 0} \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \lim_{\Delta y \to 0} \frac{j(x,y+\Delta y) - j(x,y)}{\Delta y} = \frac{\partial i}{\partial x} + \frac{\partial j}{\partial y}$$

This shows that divergence equals the sum of partial derivatives—exactly our formula!

---

## The Velocity Field

Now let's tackle the main event: the velocity field equation.

Before implementing it, we need to understand two more differential operators in addition to divergence: **gradient** and **Laplacian**.

### Gradient

$$\nabla f(x, y) = \left(\frac{\partial f}{\partial x}, \frac{\partial f}{\partial y}\right)$$

The gradient ($\nabla f$ or grad $f$) computes the direction of steepest increase. By sampling a function at infinitesimally displaced points in each direction and combining the results, we get a vector pointing "uphill."

> **Intuition: What Does Gradient Mean?**
>
> Imagine standing on a hillside. The gradient points in the direction of steepest ascent, and its magnitude tells you how steep the slope is. In fluid simulation, we use gradients to find where pressure differences will push the fluid.

### Laplacian

$$\Delta f = \nabla^2 f = \nabla \cdot \nabla f = \frac{\partial^2 f}{\partial x^2} + \frac{\partial^2 f}{\partial y^2}$$

The Laplacian (written as $\nabla^2 f$ or $\nabla \cdot \nabla f$) is a second-order differential operator. Think of it as taking the gradient (direction of increase), then computing the divergence of that result.

> **Intuition: What Does Laplacian Mean?**
>
> The Laplacian measures how different a value is from its neighbors' average:
> - **Positive Laplacian**: Value is less than neighbors' average (local minimum)
> - **Negative Laplacian**: Value is greater than neighbors' average (local maximum)
> - **Zero Laplacian**: Value equals neighbors' average
>
> This is why the Laplacian naturally describes diffusion—values tend to spread toward equilibrium with their neighbors.

For vector fields, in Cartesian coordinates, we can compute the Laplacian component-wise:

$$\nabla^2 \vec{u} = \left(\frac{\partial^2 u_x}{\partial x^2}+\frac{\partial^2 u_x}{\partial y^2}+\frac{\partial^2 u_x}{\partial z^2}, \frac{\partial^2 u_y}{\partial x^2}+\frac{\partial^2 u_y}{\partial y^2}+\frac{\partial^2 u_y}{\partial z^2}, \frac{\partial^2 u_z}{\partial x^2}+\frac{\partial^2 u_z}{\partial y^2}+\frac{\partial^2 u_z}{\partial z^2}\right)$$

Now we have all the mathematical tools needed to understand the Navier-Stokes equation!

---

## Breaking Down the Velocity Field Equation

$$\frac{\partial \vec{u}}{\partial t} = -(\vec{u} \cdot \nabla)\vec{u} + \nu \nabla^2 \vec{u} + \vec{f}$$

Where:
- $\vec{u}$ = velocity
- $\nu$ = kinematic viscosity coefficient
- $\vec{f}$ = external force

The **left side** represents velocity change over time. The **right side** has three terms:

| Term | Name | Physical Meaning |
|------|------|------------------|
| $-(\vec{u} \cdot \nabla)\vec{u}$ | Advection | Velocity carrying itself along |
| $\nu \nabla^2 \vec{u}$ | Diffusion/Viscosity | Velocity spreading to neighbors |
| $\vec{f}$ | External Force | Applied forces (user input, gravity, etc.) |

> **Key Implementation Insight**
>
> Although mathematically these terms combine in one equation, we implement them as **separate steps** executed sequentially. This is called **operator splitting** and is what makes Stable Fluids "stable"—each step can be solved robustly.

Let's implement each term, starting with external forces (since nothing happens without an initial push!).

---

## Velocity Field: External Force Term

This is the simplest part—just add external vectors to the velocity field. This could be from user input (mouse drag), gravity, or other events.

```hlsl
float visc;           // Kinematic viscosity coefficient
float dt;             // Delta time
float velocityCoef;   // Velocity force coefficient
float densityCoef;    // Density force coefficient

// xy = velocity, z = density - the fluid solver output
RWTexture2D<float4> solver;
// Density field
RWTexture2D<float>  density;
// Velocity field
RWTexture2D<float2> velocity;
// xy = previous velocity, z = previous density
// Also used for temporary storage during projection
RWTexture2D<float3> prev;
// xy = velocity source, z = density source - external input buffer
Texture2D source;

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AddSourceVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        velocity[id] += source[id].xy * velocityCoef * dt;
        prev[id] = float3(source[id].xy * velocityCoef * dt, prev[id].z);
    }
}
```

---

## Velocity Field: Diffusion/Viscosity Term

$$\nu \nabla^2 \vec{u}$$

The nabla operator acts only on what's to its right, so first we compute the vector Laplacian, then multiply by the viscosity coefficient.

The Laplacian takes the gradient and divergence of each velocity component, causing the velocity to **diffuse** (spread) to neighboring cells. The viscosity coefficient controls how fast this spreading occurs.

### The Stability Problem

If we implement this directly (explicit method), high diffusion rates can cause **oscillation** and eventually **numerical explosion**—the simulation becomes unstable and diverges.

### The Solution: Iterative Methods

To achieve stable diffusion, we use iterative methods like **Gauss-Seidel**, **Jacobi**, or **SOR**. These convert the equation into a form that converges to the correct answer through repeated iteration.

> **Why Gauss-Seidel Works**
>
> Instead of computing the new value directly, Gauss-Seidel solves: "What value would diffuse to produce my current state?" This implicit formulation is unconditionally stable—it won't explode regardless of time step or viscosity!

More iterations = more accurate results, but in real-time graphics, we prioritize framerate and visual quality over physical accuracy. Tune the iteration count based on performance and appearance.

```hlsl
#define GS_ITERATE 2  // Gauss-Seidel iterations. Directly affects performance.

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void DiffuseVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float a = dt * visc * w * h;

        [unroll]
        for (int k = 0; k < GS_ITERATE; k++) {
            velocity[id] = (prev[id].xy + a * (
                            velocity[int2(id.x - 1, id.y)] +
                            velocity[int2(id.x + 1, id.y)] +
                            velocity[int2(id.x, id.y - 1)] +
                            velocity[int2(id.x, id.y + 1)]
                            )) / (1 + 4 * a);
            SetBoundaryVelocity(id, w, h);
        }
    }
}
```

> **Understanding the Formula**
>
> The formula `(prev + a * neighbors_sum) / (1 + 4*a)` is the Gauss-Seidel solution to the diffusion equation. The `a` term represents `dt * viscosity * gridSize`, and we're solving for the velocity that, when diffused, produces the current state.

The `SetBoundaryVelocity` function handles boundary conditions. See the repository for details.

---

## Mass Conservation (Projection)

$$\nabla \cdot \vec{u} = 0$$

Before proceeding further, let's return to mass conservation. After external forces and diffusion, each cell has accumulated velocity—but mass isn't conserved yet. Some cells have net outflow (sources) while others have net inflow (sinks).

We must enforce the constraint that divergence equals zero everywhere. This step, called **projection**, makes the fluid behave realistically—outflow from one area causes inflow elsewhere, propagating the flow.

### Implementation Challenge

When computing partial derivatives on the GPU, we need neighboring cells' values to be "finalized." Using group shared memory would be faster, but cross-group access causes artifacts. So we split projection into **three separate kernel dispatches**:

1. **Step 1**: Compute divergence from velocity field
2. **Step 2**: Solve Poisson equation using Gauss-Seidel
3. **Step 3**: Subtract gradient to make divergence zero

```hlsl
// Mass Conservation Step 1: Compute divergence from velocity field
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep1(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        // Store divergence in prev.y, pressure in prev.x
        prev[id] = float3(0.0,
                    -0.5 *
                    (uvd.x * (velocity[int2(id.x + 1, id.y)].x -
                              velocity[int2(id.x - 1, id.y)].x)) +
                    (uvd.y * (velocity[int2(id.x, id.y + 1)].y -
                              velocity[int2(id.x, id.y - 1)].y)),
                    prev[id].z);

        SetBoundaryDivergence(id, w, h);
        SetBoundaryDivPositive(id, w, h);
    }
}

// Mass Conservation Step 2: Solve Poisson equation via Gauss-Seidel
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep2(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        for (int k = 0; k < GS_ITERATE; k++)
        {
            prev[id] = float3(
                        (prev[id].y + prev[uint2(id.x - 1, id.y)].x +
                                      prev[uint2(id.x + 1, id.y)].x +
                                      prev[uint2(id.x, id.y - 1)].x +
                                      prev[uint2(id.x, id.y + 1)].x) / 4,
                        prev[id].yz);
            SetBoundaryDivPositive(id, w, h);
        }
    }
}

// Mass Conservation Step 3: Make divergence zero
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep3(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float  velX, velY;
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        velX = velocity[id].x;
        velY = velocity[id].y;

        // Subtract pressure gradient
        velX -= 0.5 * (prev[uint2(id.x + 1, id.y)].x -
                       prev[uint2(id.x - 1, id.y)].x) / uvd.x;
        velY -= 0.5 * (prev[uint2(id.x, id.y + 1)].x -
                       prev[uint2(id.x, id.y - 1)].x) / uvd.y;

        velocity[id] = float2(velX, velY);
        SetBoundaryVelocity(id, w, h);
    }
}
```

> **The Pressure-Velocity Connection**
>
> This projection step finds a "pressure" field that, when we subtract its gradient from velocity, gives us a divergence-free field. Physically, this represents how pressure differences push fluid to maintain incompressibility.

Now the velocity field conserves mass—where there's outflow, inflow occurs to compensate, creating realistic fluid propagation!

---

## Velocity Field: Advection Term

$$-(\vec{u} \cdot \nabla)\vec{u}$$

Advection is where the Lagrangian perspective enters our Eulerian grid method. We need to move velocity values along with the flow.

### The Backtrace Method

For each grid cell, we trace **backward** along the velocity to find where the current value "came from" in the previous timestep. We then copy that value to the current cell.

Since the backtrace point rarely lands exactly on a grid cell, we use **bilinear interpolation** with the four nearest cells to get the correct value.

```hlsl
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AdvectVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    density.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        int ddx0, ddx1, ddy0, ddy1;
        float x, y, s0, t0, s1, t1, dfdt;

        dfdt = dt * (w + h) * 0.5;

        // Calculate backtrace point
        x = (float)id.x - dfdt * prev[id].x;
        y = (float)id.y - dfdt * prev[id].y;

        // Clamp to simulation bounds
        clamp(x, 0.5, w + 0.5);
        clamp(y, 0.5, h + 0.5);

        // Find neighboring cells for interpolation
        ddx0 = floor(x);
        ddx1 = ddx0 + 1;
        ddy0 = floor(y);
        ddy1 = ddy0 + 1;

        // Calculate interpolation weights
        s1 = x - ddx0;
        s0 = 1.0 - s1;
        t1 = y - ddy0;
        t0 = 1.0 - t1;

        // Bilinear interpolation from backtraced position
        velocity[id] = s0 * (t0 * prev[int2(ddx0, ddy0)].xy +
                             t1 * prev[int2(ddx0, ddy1)].xy) +
                       s1 * (t0 * prev[int2(ddx1, ddy0)].xy +
                             t1 * prev[int2(ddx1, ddy1)].xy);
        SetBoundaryVelocity(id, w, h);
    }
}
```

> **Why Backtrace Instead of Forward?**
>
> Forward advection (pushing values to where they'll go) causes gaps and overlaps—some cells receive multiple values, others none. Backtracing (pulling from where values came) guarantees every cell gets exactly one value. This is a key insight of Stable Fluids!

---

## The Density Field

Now let's look at the density field equation:

$$\frac{\partial \rho}{\partial t} = -(\vec{u} \cdot \nabla)\rho + \kappa \nabla^2 \rho + S$$

Where:
- $\vec{u}$ = velocity
- $\kappa$ = diffusion coefficient
- $\rho$ = density
- $S$ = external density source

The density field isn't strictly necessary, but it allows us to create visual effects like smoke or ink by having pixels "ride" on the velocity field, diffusing and flowing together.

Notice that this equation has the **same structure** as the velocity field equation:
- Advection term: $-(\vec{u} \cdot \nabla)\rho$
- Diffusion term: $\kappa \nabla^2 \rho$
- Source term: $S$

The only differences are:
1. Vectors become scalars (density is just a number)
2. Kinematic viscosity ($\nu$) becomes diffusion coefficient ($\kappa$)
3. **No mass conservation required** (density can compress/expand freely)

Since density represents concentration, not physical mass in our simulation, we don't need the divergence-free constraint. This makes the density step simpler than the velocity step.

The implementation reuses the same kernels with reduced dimensionality. See the repository for the density field implementation.

---

## Simulation Step Order

Here's the complete simulation loop order:

### Each Frame:

1. **Apply external forces** to velocity and density fields

2. **Update velocity field:**
   - Diffusion (viscosity)
   - **Mass conservation (projection)**
   - Advection
   - **Mass conservation (projection)**

3. **Update density field:**
   - Diffusion
   - Advection (using velocity field)

> **Why Project Twice?**
>
> We apply mass conservation after both diffusion AND advection because each step can introduce divergence. The second projection ensures the final velocity field is truly divergence-free.

---

## C# Controller: Dispatching the Kernels

The C# code orchestrates these kernel dispatches in the correct order:

```csharp
protected override void VelocityStep()
{
    // Add velocity source to velocity field
    if (SorceTex != null)
    {
        computeShader.SetTexture(kernelMap[ComputeKernels.AddSourceVelocity], sourceId, SorceTex);
        computeShader.SetTexture(kernelMap[ComputeKernels.AddSourceVelocity], velocityId, velocityTex);
        computeShader.SetTexture(kernelMap[ComputeKernels.AddSourceVelocity], prevId, prevTex);
        computeShader.Dispatch(kernelMap[ComputeKernels.AddSourceVelocity],
            Mathf.CeilToInt(velocityTex.width / gpuThreads.x),
            Mathf.CeilToInt(velocityTex.height / gpuThreads.y), 1);
    }

    // Diffuse velocity
    // ... dispatch DiffuseVelocity

    // Project (3 steps)
    // ... dispatch ProjectStep1, ProjectStep2, ProjectStep3

    // Swap buffers
    // ... dispatch SwapVelocity

    // Advect velocity
    // ... dispatch AdvectVelocity

    // Project again (3 steps)
    // ... dispatch ProjectStep1, ProjectStep2, ProjectStep3
}
```

---

## Results

Running the simulation and dragging the mouse on screen produces fluid simulation like this:

![Execution Result](images/fluid-result.png)
*Figure: Execution result*

---

## Summary

Fluid simulation is computationally expensive—unlike pre-rendered effects, real-time game engines face significant performance constraints. However, with improving GPU capabilities, 2D simulations at reasonable resolutions can now achieve acceptable framerates.

### Performance Optimization Ideas

- Replace Gauss-Seidel iterations with faster alternatives
- Substitute the velocity field with curl noise for lighter computation
- Reduce grid resolution and upscale results
- Skip projection steps for artistic (non-physical) effects

### Going Further

If this chapter sparked your interest in fluids, definitely try the next chapter on **Particle-Based Fluid Simulation (SPH)**. It approaches fluid from a completely different angle, giving you a deeper appreciation for the richness and implementation challenges of fluid simulation.

---

## References

- Jos Stam, SIGGRAPH 1999, "Stable Fluids"

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **Eulerian vs Lagrangian**: Fixed grid vs. moving particles—fundamental fluid perspectives
>
> 2. **Navier-Stokes terms**: Force → Diffusion → Projection → Advection → Projection
>
> 3. **Divergence = 0**: Incompressible fluids have no sources or sinks
>
> 4. **Gauss-Seidel**: Iterative solver for stable diffusion and projection
>
> 5. **Backtracing**: Pull values from where they came (not push where they're going)
>
> 6. **Operator splitting**: Solve complex equations by tackling one term at a time
>
> 7. **Project twice**: After diffusion AND after advection to maintain divergence-free velocity

---

## Mathematical Operator Quick Reference

| Operator | Symbol | Formula (2D) | Meaning |
|----------|--------|--------------|---------|
| **Gradient** | $\nabla f$ | $(\frac{\partial f}{\partial x}, \frac{\partial f}{\partial y})$ | Direction of steepest increase |
| **Divergence** | $\nabla \cdot \vec{u}$ | $\frac{\partial u}{\partial x} + \frac{\partial v}{\partial y}$ | Net outflow from a point |
| **Laplacian** | $\nabla^2 f$ | $\frac{\partial^2 f}{\partial x^2} + \frac{\partial^2 f}{\partial y^2}$ | Difference from neighbors' average |

---

*Next chapter: SPH Particle-Based Fluid Simulation!*
