# Chapter 6: Curl Noise - Pseudo-Fluid Noise Algorithm

**Author**: Sakota

**Sample Project**: "CurlNoise" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter explains the GPU implementation of **Curl Noise**, a pseudo-fluid algorithm. Curl Noise provides fluid-like motion at significantly lower computational cost than full fluid simulations like Navier-Stokes equations.

With the increasing need for real-time rendering at 4K and 8K resolutions, lightweight algorithms like Curl Noise become valuable choices for achieving fluid-like effects on lower-spec machines or at high resolutions.

---

## What is Curl Noise?

Curl Noise was published in 2007 by Professor Robert Bridson of the University of British Columbia (also known for the FLIP fluid simulation method). While previous volumes covered Navier-Stokes fluid simulation, Curl Noise offers a pseudo-fluid alternative with much lower computational requirements.

---

## Understanding Velocity Fields

Fluid simulation fundamentally requires a **velocity field** - a vector field where each point in space has an associated velocity vector.

### 2D Velocity Field Visualization

Imagine a 2D plane where each small region has its own direction and magnitude arrow. In 3D, picture a cube subdivided into tiny blocks, each with its own velocity vector.

---

## The Curl Noise Algorithm

### The Core Insight

Curl Noise cleverly uses **gradient noise** (like Perlin or Simplex Noise) as a **potential field**, then derives a velocity field from it. This chapter uses 3D Simplex Noise as the potential field.

### The Mathematical Formula

The Curl Noise algorithm is expressed as:

$$\vec{u} = \nabla \times \psi$$

Where:
- $\vec{u}$ = resulting velocity vector
- $\nabla$ = vector differential operator (nabla)
- $\psi$ = potential field (3D Simplex Noise)

The right side is the **cross product** of the nabla operator and the potential field - mathematically known as **rot A** (rotation/curl).

### Expanded Form

Computing the cross product with partial derivatives:

$$\vec{u} = \left( \frac{\partial \psi_3}{\partial y} - \frac{\partial \psi_2}{\partial z}, \frac{\partial \psi_1}{\partial z} - \frac{\partial \psi_3}{\partial x}, \frac{\partial \psi_2}{\partial x} - \frac{\partial \psi_1}{\partial y} \right)$$

### Intuitive Understanding

The rotation (curl) operation can be understood as: "twisted partial derivative lookups in each direction, with terms subtracted from each other to create rotation."

The implementation samples the noise field at slightly offset positions in each axis direction, then performs the cross product calculation.

---

## Mass Conservation (Divergence-Free)

Those familiar with fluid dynamics might wonder about **mass conservation** - the principle that fluid flow must have zero divergence (what flows in must flow out):

$$\nabla \cdot \vec{u} = 0$$

The elegant answer: gradient noise inherently varies smoothly (like a 2D gradient where darker pixels on one side mean lighter on the other). This smooth variation naturally guarantees divergence-free behavior in the potential field. The mathematical structure ensures mass conservation without explicit enforcement.

---

## GPU Implementation

The algorithm translates to remarkably simple shader code:

```hlsl
#define EPSILON 1e-3

float3 CurlNoise(float3 coord)
{
    float3 dx = float3(EPSILON, 0.0, 0.0);
    float3 dy = float3(0.0, EPSILON, 0.0);
    float3 dz = float3(0.0, 0.0, EPSILON);

    // Sample noise at offset positions
    float3 dpdx0 = snoise(coord - dx);
    float3 dpdx1 = snoise(coord + dx);
    float3 dpdy0 = snoise(coord - dy);
    float3 dpdy1 = snoise(coord + dy);
    float3 dpdz0 = snoise(coord - dz);
    float3 dpdz1 = snoise(coord + dz);

    // Cross product calculation
    float x = dpdy1.z - dpdy0.z + dpdz1.y - dpdz0.y;
    float y = dpdz1.x - dpdz0.x + dpdx1.z - dpdx0.z;
    float z = dpdx1.y - dpdx0.y + dpdy1.x - dpdy0.x;

    return float3(x, y, z) / EPSILON * 2.0;
}
```

### Implementation Breakdown

| Step | Description |
|------|-------------|
| Define EPSILON | Small offset for finite difference approximation |
| Sample noise at 6 positions | +/- offset in each axis direction |
| Compute cross product terms | Difference of perpendicular components |
| Scale result | Divide by epsilon and scale |

---

## Practical Applications

The sample Compute Shader implementation demonstrates various effects:

1. **Particle Advection** - Move particles along the curl noise velocity field
2. **Fire/Smoke Effects** - Add upward bias vectors to create flame-like appearance
3. **Abstract Fluid Art** - Creative applications limited only by imagination

### Visual Examples

The repository includes examples showing:
- Swirling particle systems
- Flame-like upward flows
- Organic, flowing motion patterns

---

## Key Advantages

| Aspect | Benefit |
|--------|---------|
| **Computational Cost** | Much lighter than full fluid simulation |
| **Implementation** | Simple arithmetic operations |
| **Scalability** | Runs well at high resolutions |
| **Quality** | Produces convincing fluid-like motion |
| **Divergence-Free** | Guaranteed by mathematical structure |

---

## Key Takeaways

1. **Curl Noise derives velocity from noise** - Uses gradient noise as potential field

2. **Cross product creates rotation** - The curl operation naturally produces swirling motion

3. **Inherently divergence-free** - No explicit mass conservation step needed

4. **Extremely efficient** - Just noise samples and arithmetic

5. **GPU-friendly** - Parallelizes perfectly in compute shaders

6. **High resolution capable** - Ideal for 4K/8K real-time rendering

---

## Summary

Curl Noise enables 3D pseudo-fluid effects with minimal computational overhead. For high-resolution real-time rendering, it's an invaluable tool in the graphics programmer's arsenal.

This chapter concludes with gratitude to Professor Robert Bridson, who continues to develop innovative techniques like this algorithm.

---

## References

- Robert Bridson, Jim Hourihan, Marcus Nordenstam. 2007. "Curl-noise for procedural fluid flow." In Proceedings of ACM SIGGRAPH 46.

---

*Note: Curl Noise is a powerful starting point - combine it with additional forces, boundaries, and particle systems to create sophisticated fluid-like effects.*
