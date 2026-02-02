# Chapter 10: MultiPlane Perspective Projection

*Original author: @fuqunaga*
*Translation and annotations by Claude*

---

## Introduction

This chapter explains how to project imagery onto multiple surfaces (walls, floor) of a rectangular room using projectors, creating an immersive experience where viewers feel they're inside a CG world.

We'll cover the underlying theory of CG camera processing and its practical applications.

Sample project is in **Assets/RoomProjection** at:
https://github.com/IndieVisualLab/UnityGraphicsProgramming

> **What You'll Learn**
>
> - How CG cameras work (coordinate transformations)
> - How to match perspective across multiple cameras
> - Deriving and modifying projection matrices
> - Practical room-scale projection mapping

---

## How CG Cameras Work

A CG camera performs **perspective projection**—transforming 3D models within a visible region into a 2D image.

This involves a sequence of coordinate system transformations:

```
Local Space → World Space → View Space → Clip Space → NDC → Screen Space
```

| Coordinate System | Description |
|-------------------|-------------|
| **Local Space** | Each model's origin at its own center |
| **World Space** | Shared origin for the entire scene |
| **View Space** | Camera at origin, looking down -Z |
| **Clip Space** | 4D homogeneous coordinates (x,y,z,w) for clipping |
| **NDC** | Normalized Device Coordinates [-1,1] range |
| **Screen Space** | 2D pixel coordinates on output display |

> **Key Insight: Matrix Composition**
>
> Each transformation can be represented as a matrix. By multiplying matrices together in advance, we can perform multiple coordinate transformations with a single matrix-vector multiplication—a massive performance win on GPUs.

---

## Matching Perspective Across Multiple Cameras

### The View Frustum

A camera's **view frustum** is a truncated pyramid (frustum) that represents the 3D volume the camera can see:
- **Apex**: Camera position
- **Base**: The projection plane (screen)
- **Sides**: Field of view boundaries

### Connecting Frustums

When two camera frustums:
1. Share the same apex (camera position)
2. Have adjacent sides that touch

...their projected images will connect seamlessly with matching perspective!

> **Why This Works**
>
> Think of the frustum as a bundle of rays emanating from the camera. If two frustums share an apex and their boundaries touch, the rays form a continuous set—no gaps, no overlaps. The perspective is consistent because all rays originate from the same point.

### Five-Camera Room Setup

By extending this to 5 cameras (4 walls + floor), with all frustums sharing one apex and touching at their edges, we can generate images for each room surface that create a cohesive immersive environment.

```
        ┌─────────┐
        │  Back   │
  ┌─────┼─────────┼─────┐
  │Left │  Floor  │Right│
  └─────┼─────────┼─────┘
        │  Front  │
        └─────────┘
```

When viewed from the apex (the shared viewpoint), every direction shows perspective-correct imagery.

> **Why Not 6 Faces?**
>
> Theoretically possible, but the ceiling is often used for projector mounting. The sample assumes 5 faces (excluding ceiling).

---

## Deriving the Projection Matrix

The **projection matrix** (Proj) transforms from View Space to Clip Space:

- **V**: Position in View Space
- **C**: Position in Clip Space

$$C = Proj \times V$$

To get NDC (Normalized Device Coordinates), divide by the w component:

$$NDC = \left(\frac{C_x}{C_w}, \frac{C_y}{C_w}, \frac{C_z}{C_w}\right)$$

The projection matrix is constructed so that $C_w = -V_z$ (negative because View Space looks down -Z).

The division by $V_z$ creates the perspective effect—distant objects (larger |z|) get scaled down.

### Building the Matrix

Let's define:
- **N**: Top-right corner of the near clip plane
- **F**: Top-right corner of the far clip plane

**For X scaling:**

We want the view to map to NDC range [-1, 1], and we know we'll divide by $C_w = -V_z$:

$$Proj[0,0] = \frac{N_z}{N_x}$$

**For Y scaling (same logic):**

$$Proj[1,1] = \frac{N_z}{N_y}$$

**For Z (depth mapping):**

This is trickier. The Z calculation is:

$$C_z = Proj[2,2] \times V_z + Proj[2,3] \times V_w \quad \text{(where } V_w = 1\text{)}$$

$$NDC_z = \frac{C_z}{C_w} \quad \text{(where } C_w = -V_z\text{)}$$

We want $N_z \rightarrow -1$ and $F_z \rightarrow 1$. Setting up equations:

$$-1 = \frac{1}{N_z}(a \cdot N_z + b)$$
$$1 = \frac{1}{F_z}(a \cdot F_z + b)$$

Solving this system:

$$Proj[2,2] = a = \frac{F_z + N_z}{F_z - N_z}$$
$$Proj[2,3] = b = \frac{-2 F_z N_z}{F_z - N_z}$$

**For W (perspective divide setup):**

$$Proj[3,2] = -1$$

### The Complete Projection Matrix

$$Proj = \begin{pmatrix}
\frac{N_z}{N_x} & 0 & 0 & 0 \\
0 & \frac{N_z}{N_y} & 0 & 0 \\
0 & 0 & \frac{F_z+N_z}{F_z-N_z} & \frac{-2F_zN_z}{F_z-N_z} \\
0 & 0 & -1 & 0
\end{pmatrix}$$

> **Understanding the Matrix**
>
> - **Row 0**: X scaling (based on horizontal FOV)
> - **Row 1**: Y scaling (based on vertical FOV)
> - **Row 2**: Z remapping (near→-1, far→1)
> - **Row 3**: Perspective divide setup ($C_w = -V_z$)

---

## Unity's Projection Matrix Gotcha

If you've worked with projection matrices in Unity shaders, some of this might seem off. Here's why:

**Camera.projectionMatrix** follows OpenGL conventions:
- NDC z range: [-1, 1]
- $C_w = -V_z$

However, Unity converts this to platform-specific formats when passing to shaders. Different platforms handle Z-buffer depth differently:
- Some use [0, 1] range
- Some reverse the direction

This is a common source of bugs. When working with projection matrices directly, use `GL.GetGPUProjectionMatrix()` to get the platform-correct version.

---

## Manipulating the Frustum

### Matching Projection Size to Room Faces

The frustum's base (projection plane) shape depends on:
- **Field of View (fov)**: Vertical angle
- **Aspect Ratio**: Width/height ratio

Unity's camera exposes FOV in the Inspector, but aspect ratio must be set via code.

Given the **face size** (wall dimensions) and **distance** (viewpoint to wall):

```csharp
camera.aspect = faceSize.x / faceSize.y;
camera.fieldOfView = 2f * Mathf.Atan2(faceSize.y * 0.5f, distance)
                     * Mathf.Rad2Deg;
```

> **Breaking This Down**
>
> - `Atan2(opposite, adjacent)` gives the angle in radians
> - We use half the height because FOV is measured from center
> - Multiply by 2 to get full vertical FOV
> - Convert to degrees for Unity's API

### Lens Shift (Off-Center Projection)

What if the viewpoint isn't centered on the room? We need to shift the projection plane horizontally or vertically while keeping the same FOV.

This is called **lens shift**—equivalent to what physical projectors do when adjusting image position without moving the projector.

**How it works in the projection matrix:**

You might think to modify Proj[0,3] and Proj[1,3] (the translation components), but remember—we divide by $C_w$ afterward. To shift in NDC space, modify Proj[0,2] and Proj[1,2]:

$$Proj = \begin{pmatrix}
\frac{N_z}{N_x} & 0 & LensShift_x & 0 \\
0 & \frac{N_z}{N_y} & LensShift_y & 0 \\
0 & 0 & \frac{F_z+N_z}{F_z-N_z} & \frac{-2F_zN_z}{F_z-N_z} \\
0 & 0 & -1 & 0
\end{pmatrix}$$

**Implementation:**

```csharp
// Convert position offset to NDC units [-1, 1]
var shift = new Vector2(
    positionOffset.x / faceSize.x,
    positionOffset.y / faceSize.y
) * 2f;

var projectionMatrix = camera.projectionMatrix;
projectionMatrix[0, 2] = shift.x;
projectionMatrix[1, 2] = shift.y;
camera.projectionMatrix = projectionMatrix;
```

> **Important Warning**
>
> Once you set `Camera.projectionMatrix` directly, changes to `Camera.fieldOfView` are ignored until you call `Camera.ResetProjectionMatrix()`. The camera remembers that you've overridden its projection.

---

## Room Projection System

Putting it all together for an immersive room:

**Given:**
- Rectangular room dimensions
- Viewer position (tracked in real-time)

**For each surface (4 walls + floor):**
1. Calculate distance from viewer to surface
2. Set camera FOV and aspect to match surface dimensions
3. Apply lens shift based on viewer offset from surface center
4. Render to texture
5. Project texture onto physical surface

When the viewer stands at the tracked position, all surfaces show perspective-correct imagery—creating the illusion of being inside the CG world.

---

## Practical Considerations

This technique creates a form of VR without a headset—surrounding the viewer with reactive imagery. However, some limitations exist:

**Challenges:**
- No stereoscopic depth (both eyes see the same image)
- Viewer can see the flat projection surfaces

**Mitigations:**
- **Increase room size**: Greater distance reduces binocular disparity effects
- **Control lighting**: Dark content minimizes surface reflections
- **Use matte materials**: Avoid specular reflections that reveal the flat surface
- **Track viewer accurately**: Perspective breaks if tracking is off

> **CAVE Systems**
>
> Professional installations called **CAVE** (Cave Automatic Virtual Environment) combine this multi-plane projection with stereoscopic displays for true 3D immersion. The viewer wears lightweight stereo glasses while surrounded by tracked projection surfaces.

---

## Summary

This chapter demonstrated how understanding projection matrices enables creative camera configurations:

1. **Coordinate transformations** convert 3D scenes to 2D images
2. **View frustums** can be arranged to create seamless multi-screen displays
3. **Projection matrix manipulation** allows custom FOV and lens shift
4. **Room-scale projection** creates immersive environments

The math behind CG cameras isn't just academic—it's the foundation for practical applications from projection mapping to VR to architectural visualization.

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **Coordinate pipeline**: Local → World → View → Clip → NDC → Screen
>
> 2. **Projection matrix**: Transforms View Space to Clip Space, creating perspective
>
> 3. **Perspective divide**: Dividing by w ($-V_z$) makes distant objects smaller
>
> 4. **Matching frustums**: Same apex + touching edges = seamless perspective
>
> 5. **FOV calculation**: `2 * atan2(halfHeight, distance)` in radians
>
> 6. **Lens shift**: Modify Proj[0,2] and Proj[1,2], not [0,3] and [1,3]
>
> 7. **Platform differences**: Unity's projection matrix varies by platform (OpenGL vs D3D depth range)
>
> 8. **ResetProjectionMatrix()**: Required after manual projection matrix changes before FOV changes take effect

---

## Projection Matrix Reference

| Element | Purpose | Formula |
|---------|---------|---------|
| Proj[0,0] | X scale | $N_z / N_x$ |
| Proj[1,1] | Y scale | $N_z / N_y$ |
| Proj[2,2] | Z remap (scale) | $(F_z + N_z) / (F_z - N_z)$ |
| Proj[2,3] | Z remap (offset) | $-2 F_z N_z / (F_z - N_z)$ |
| Proj[3,2] | W setup | $-1$ |
| Proj[0,2] | Lens shift X | User-defined |
| Proj[1,2] | Lens shift Y | User-defined |

---

## References

- Unity Projection Matrix Documentation - https://docs.unity3d.com/ScriptReference/Camera-projectionMatrix.html
- Platform Rendering Differences - https://docs.unity3d.com/Manual/SL-PlatformDifferences.html
- CAVE Systems - https://en.wikipedia.org/wiki/Cave_automatic_virtual_environment

---

*This concludes Volume 1 of Unity Graphics Programming!*
