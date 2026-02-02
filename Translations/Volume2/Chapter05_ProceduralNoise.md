# Chapter 5: Introduction to Procedural Noise

**Author**: Oishi

**Sample Project**: "TheStudyOfProceduralNoise" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter provides a comprehensive explanation of noise algorithms used in computer graphics. Noise was developed in the 1980s as a new technique for procedural texture generation. Early computers had very limited memory, making stored texture images impractical. Procedural noise offered an elegant solution.

Natural phenomena like terrain, clouds, water, fire, marble, wood grain, and crystals exhibit both visual complexity and regular patterns. Noise can generate texture patterns optimal for representing these natural elements, making it an indispensable technique for procedural graphics.

The most notable noise algorithms are **Perlin Noise** and **Simplex Noise**, both achievements of **Ken Perlin**.

---

## What is Noise?

In computer graphics, noise refers to a function that takes an N-dimensional vector as input and returns a scalar value with these characteristics:

- **Continuous** - Changes smoothly between adjacent regions
- **Isotropic** - Statistically invariant under rotation (rotating a region doesn't change its characteristics)
- **Translation invariant** - Statistically invariant under translation
- **Band-limited** - Energy is concentrated in a specific frequency spectrum

### Applications by Dimension

| Dimensions | Input | Use Case |
|------------|-------|----------|
| 1D | Time | Animation |
| 2D | UV coordinates | Textures |
| 3D | UV + Time | Animated textures |
| 3D | Local coordinates | Solid/volumetric textures |
| 4D | Local coords + Time | Animated volumetric textures |

---

## Value Noise

The simplest noise algorithm, ideal for understanding the fundamentals.

### Algorithm

1. Define a grid with evenly spaced lattice points
2. Calculate pseudo-random values at each lattice point
3. Interpolate values between lattice points

### Implementation Details

**Grid Calculation**:
```hlsl
// Integer part (lattice coordinates)
float2 i = floor(v);
// Fractional part (position within cell)
float2 f = frac(v);

// Four corner coordinates
float2 i00 = i;
float2 i10 = i + float2(1.0, 0.0);
float2 i01 = i + float2(0.0, 1.0);
float2 i11 = i + float2(1.0, 1.0);
```

**Pseudo-Random Function**:
```hlsl
float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}
```

**Hermite Interpolation** (smoother than linear):
```hlsl
// 3rd-degree Hermite curve: 3t^2 - 2t^3
float2 interpolate(float2 t)
{
    return t * t * (3.0 - 2.0 * t);
}
```

**Final Interpolation**:
```hlsl
float2 u = interpolate(f);
return lerp(lerp(n00, n10, u.x), lerp(n01, n11, u.x), u.y);
```

**Limitation**: Value Noise shows visible grid artifacts because it lacks true isotropy.

---

## Perlin Noise (Gradient Noise)

Developed by Ken Perlin for the 1982 film "Tron" and published at SIGGRAPH 1985 in "An Image Synthesizer."

### Key Difference from Value Noise

Instead of random scalar values at lattice points, Perlin Noise uses **gradient vectors**.

### Algorithm

1. Calculate lattice coordinates
2. Generate gradient vectors at each lattice point
3. Calculate vectors from lattice points to target point P
4. Compute dot products between gradients and offset vectors
5. Interpolate results using Hermite curves

### The Dot Product Insight

The dot product measures how aligned two vectors are:
- Same direction: +1
- Perpendicular: 0
- Opposite: -1

When the gradient aligns with the offset vector, noise values are high; when opposed, values are low.

### Implementation

```hlsl
float originalPerlinNoise(float2 v)
{
    float2 i = floor(v);
    float2 f = frac(v);

    // Lattice corner coordinates
    float2 i00 = i;
    float2 i10 = i + float2(1.0, 0.0);
    float2 i01 = i + float2(0.0, 1.0);
    float2 i11 = i + float2(1.0, 1.0);

    // Offset vectors from corners to point
    float2 p00 = f;
    float2 p10 = f - float2(1.0, 0.0);
    float2 p01 = f - float2(0.0, 1.0);
    float2 p11 = f - float2(1.0, 1.0);

    // Gradient vectors (normalized random)
    float2 g00 = normalize(pseudoRandom(i00));
    float2 g10 = normalize(pseudoRandom(i10));
    float2 g01 = normalize(pseudoRandom(i01));
    float2 g11 = normalize(pseudoRandom(i11));

    // Dot products
    float n00 = dot(g00, p00);
    float n10 = dot(g10, p10);
    float n01 = dot(g01, p01);
    float n11 = dot(g11, p11);

    // Interpolation
    float2 u_xy = interpolate(f.xy);
    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), u_xy.x);
    return lerp(n_x.x, n_x.y, u_xy.y);
}
```

---

## Improved Perlin Noise

Published by Ken Perlin in 2001 to address two issues in the original algorithm.

### Improvement 1: 5th-Degree Hermite Curve

The original 3rd-degree curve has discontinuous second derivatives at cell boundaries, causing visual artifacts in bump mapping.

**5th-Degree Hermite Curve**:
```
f(t) = 6t^5 - 15t^4 + 10t^3
```

This ensures both first and second derivatives are zero at t=0 and t=1, creating smooth continuity.

### Improvement 2: Constrained Gradient Vectors

In 3D, random gradients can cluster along axes, causing spots. The improved version uses exactly 12 gradient directions:

```
(1,1,0), (-1,1,0), (1,-1,0), (-1,-1,0),
(1,0,1), (-1,0,1), (1,0,-1), (-1,0,-1),
(0,1,1), (0,-1,1), (0,1,-1), (0,-1,-1)
```

These point to the edges of a cube, providing good distribution while simplifying dot product calculations.

---

## Simplex Noise

Also by Ken Perlin (2001), designed as a superior alternative to classic Perlin Noise.

### Advantages Over Perlin Noise

| Aspect | Perlin Noise | Simplex Noise |
|--------|--------------|---------------|
| Complexity | O(2^N) | O(N^2) |
| Higher dimensions | Exponential cost increase | Polynomial cost increase |
| Visual artifacts | Some directional bias | Minimal artifacts |
| Hardware efficiency | Moderate | Excellent |

### The Simplex Grid

Instead of hypercubes (squares, cubes), Simplex Noise uses **simplices** - the simplest shapes that tile N-dimensional space:
- 1D: Line segments
- 2D: Equilateral triangles
- 3D: Tetrahedra
- 4D: Pentatopes (5-cell)

An N-dimensional simplex has only N+1 vertices, compared to 2^N for a hypercube.

### Determining Which Simplex Contains Point P

1. **Skew** the input space to align simplices with a regular grid
2. **Floor** to find which simplex unit cell
3. **Compare** coordinate magnitudes to determine which simplex within the unit

### Summation Instead of Interpolation

Rather than interpolating between corners, Simplex Noise sums contributions from each corner, using a radially-decaying influence function:

```
Each corner's contribution = gradient_extrapolation * radial_falloff
Total = sum of all corner contributions
```

### Implementation Highlights

**Permutation Polynomial** (replaces lookup tables):
```hlsl
// Generates pseudo-random indices without tables
float3 permute(float3 x)
{
    return fmod(((x * 34.0) + 1.0) * x, 289.0);
}
```

**Cross-Polytope Gradients**:

Gradients are distributed on the surface of:
- 2D: Square (diamond orientation)
- 3D: Octahedron
- 4D: 16-cell (truncated)

**Taylor Series Normalization**:
```hlsl
// Fast inverse square root approximation
float3 taylorInvSqrt(float3 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}
```

---

## Sample Code Locations

```
TheStudyOfProceduralNoise/
├── Scenes/
│   ├── ShaderExampleList    # View all noise types
│   └── CompareBumpmap       # Compare interpolation curves
└── Shaders/ProceduralNoise/
    ├── ValueNoise2D.cginc
    ├── ValueNoise3D.cginc
    ├── OriginalPerlinNoise2D.cginc
    ├── ClassicPerlinNoise2D.cginc  # Improved Perlin
    ├── SimplexNoise2D.cginc
    └── [3D and 4D variants...]
```

---

## Key Takeaways

1. **Value Noise** is simple but shows grid artifacts
2. **Perlin Noise** uses gradients for isotropy
3. **Improved Perlin** fixes bump mapping artifacts with 5th-degree curves
4. **Simplex Noise** scales better to higher dimensions
5. **All noise types** share the pattern: lattice + randomness + interpolation
6. **Performance matters** - noise runs per-pixel, so efficiency is crucial

---

## References

1. "An Image Synthesizer" - Ken Perlin, SIGGRAPH 1985
2. "Improving Noise" - Ken Perlin, 2002
3. "Simplex noise demystified" - Stefan Gustavson, 2005
4. "Efficient computational noise in GLSL" - McEwan et al., 2012
5. Reference implementation: [http://mrl.nyu.edu/~perlin/noise/](http://mrl.nyu.edu/~perlin/noise/)

---

*The next chapter demonstrates practical noise application with Curl Noise for pseudo-fluid effects.*
