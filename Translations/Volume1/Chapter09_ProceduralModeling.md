# Chapter 9: Introduction to Procedural Modeling in Unity

*Original author: @mattatz*
*Translation and annotations by Claude*

---

## Introduction

**Procedural Modeling** is a technique for constructing 3D models using rules and algorithms rather than manual manipulation.

Traditional modeling involves using software like Blender or 3ds Max to manually manipulate vertices and edges to achieve a desired shape. In contrast, procedural modeling describes rules that automatically generate shapes through a series of computations.

> **Why Procedural Modeling Matters**
>
> Procedural modeling is used across many domains:
> - **Games**: Terrain generation, vegetation, city building (enabling stages that change with each playthrough)
> - **Architecture**: Grasshopper plugin for Rhinoceros CAD enables parametric design
> - **Film/VFX**: Generating crowds, forests, cities at scale
> - **3D Printing**: Creating complex structures impossible to model manually

Procedural modeling enables two key capabilities:

### 1. Parametric Structures

A parametric structure can be transformed by adjusting parameters. For example, a sphere model can have:
- **radius**: Controls size
- **segments**: Controls smoothness

Once you implement a parametric structure, you can instantly generate variations for any use case.

### 2. Runtime-Generated Content

Rather than importing pre-built models, procedural techniques can generate models in real-time:
- Trees that grow toward the sun at any position
- Cities that build up as the user clicks locations
- Reduced data size (generate variations instead of storing them)

Master procedural modeling, and you can even build your own modeling tools!

---

## Sample Repository

Sample code is in **Assets/ProceduralModeling** at:
https://github.com/IndieVisualLab/UnityGraphicsProgramming

The C# scripts in `Assets/ProceduralModeling/Scripts` are the focus of this chapter.

---

## Mesh Representation in Unity

Unity manages geometry data through the **Mesh** class.

A model's shape is composed of triangles in 3D space, where each triangle is defined by 3 vertices. From the Unity documentation:

> In the Mesh class, all vertices are stored in a single array, and each triangle is specified by three integers corresponding to vertex array indices. Triangles are collected into an integer array, grouped in threes from the beginning—elements 0, 1, 2 define the first triangle, elements 3, 4, 5 define the second, and so on.

Each vertex can have associated data:
- **UV coordinates**: For texture mapping
- **Normals**: For lighting calculations

> **Mental Model**
>
> Think of a Mesh as:
> - A bag of vertices (positions in 3D space)
> - A list of triangles (triplets of vertex indices)
> - Per-vertex attributes (UVs, normals, colors, tangents)
>
> The triangles index into the vertex array—vertices can be shared across multiple triangles.

---

## Quad: The Simplest Mesh

Let's start with the most basic shape: a **Quad** (4 vertices, 2 triangles forming a square).

```
    0 -------- 1
    |  \       |
    |    \     |
    |      \   |
    3 -------- 2
```

The numbers represent vertex indices. Two triangles: (0,1,2) and (2,3,0).

### Sample: Quad.cs

**Step 1: Create Mesh instance**

```csharp
var mesh = new Mesh();
```

**Step 2: Define vertex data**

```csharp
// Half size for centering the quad at origin
var hsize = size * 0.5f;

// Quad vertex positions
var vertices = new Vector3[] {
    new Vector3(-hsize,  hsize, 0f), // 0: top-left
    new Vector3( hsize,  hsize, 0f), // 1: top-right
    new Vector3( hsize, -hsize, 0f), // 2: bottom-right
    new Vector3(-hsize, -hsize, 0f)  // 3: bottom-left
};

// UV coordinates (texture mapping)
var uv = new Vector2[] {
    new Vector2(0f, 0f), // vertex 0
    new Vector2(1f, 0f), // vertex 1
    new Vector2(1f, 1f), // vertex 2
    new Vector2(0f, 1f)  // vertex 3
};

// Normals (all pointing toward camera, -Z direction)
var normals = new Vector3[] {
    new Vector3(0f, 0f, -1f),
    new Vector3(0f, 0f, -1f),
    new Vector3(0f, 0f, -1f),
    new Vector3(0f, 0f, -1f)
};
```

**Step 3: Define triangles**

```csharp
// Triangle indices (3 indices per triangle)
var triangles = new int[] {
    0, 1, 2, // First triangle (top-right)
    2, 3, 0  // Second triangle (bottom-left)
};
```

> **Winding Order**
>
> Triangle vertex order matters! Unity uses clockwise winding for front-facing triangles. If you specify vertices counter-clockwise, the triangle faces away from the camera and may be culled.

**Step 4: Assign to Mesh**

```csharp
mesh.vertices = vertices;
mesh.uv = uv;
mesh.normals = normals;
mesh.triangles = triangles;

// Calculate bounding box (needed for culling)
mesh.RecalculateBounds();

return mesh;
```

### ProceduralModelingBase

The sample code uses a `ProceduralModelingBase` base class. When parameters change, it automatically regenerates the Mesh and applies it to the MeshFilter—providing instant visual feedback in the Editor.

The `ProceduralModelingMaterial` enum lets you visualize UV coordinates and normals for debugging.

---

## Primitive Shapes

Now let's build more complex primitives.

### Plane

A **Plane** is a grid of Quads:

```
  widthSegments = 4
  ┌───┬───┬───┐
  │   │   │   │  heightSegments = 3
  ├───┼───┼───┤
  │   │   │   │
  └───┴───┴───┘
```

Parameters:
- `widthSegments`: Vertices along width
- `heightSegments`: Vertices along height
- `width`: Total width
- `height`: Total height

### Sample: Plane.cs

**Generate vertex grid:**

```csharp
var vertices = new List<Vector3>();
var uv = new List<Vector2>();
var normals = new List<Vector3>();

// Inverse of segment counts for calculating [0,1] ratios
var winv = 1f / (widthSegments - 1);
var hinv = 1f / (heightSegments - 1);

for(int y = 0; y < heightSegments; y++) {
    var ry = y * hinv;  // Row ratio [0,1]

    for(int x = 0; x < widthSegments; x++) {
        var rx = x * winv;  // Column ratio [0,1]

        vertices.Add(new Vector3(
            (rx - 0.5f) * width,   // Center on X
            0f,                     // Flat on Y
            (0.5f - ry) * height   // Center on Z
        ));
        uv.Add(new Vector2(rx, ry));
        normals.Add(new Vector3(0f, 1f, 0f));  // All point up
    }
}
```

**Generate triangles:**

```csharp
var triangles = new List<int>();

for(int y = 0; y < heightSegments - 1; y++) {
    for(int x = 0; x < widthSegments - 1; x++) {
        int index = y * widthSegments + x;
        var a = index;
        var b = index + 1;
        var c = index + 1 + widthSegments;
        var d = index + widthSegments;

        // Two triangles per grid cell
        triangles.Add(a);
        triangles.Add(b);
        triangles.Add(c);

        triangles.Add(c);
        triangles.Add(d);
        triangles.Add(a);
    }
}
```

> **Index Calculation**
>
> For a 2D grid stored in a 1D array:
> `index = y * width + x`
>
> This is a fundamental pattern you'll use constantly in graphics programming.

### ParametricPlaneBase

By varying each vertex's Y coordinate based on UV position, you can create terrain:

```csharp
protected override Mesh Build() {
    var mesh = base.Build();  // Generate base plane
    var vertices = mesh.vertices;

    var winv = 1f / (widthSegments - 1);
    var hinv = 1f / (heightSegments - 1);

    for(int y = 0; y < heightSegments; y++) {
        var ry = y * hinv;
        for(int x = 0; x < widthSegments; x++) {
            var rx = x * winv;
            int index = y * widthSegments + x;

            // Override Y with height function
            vertices[index].y = Depth(rx, ry);
        }
    }

    mesh.vertices = vertices;
    mesh.RecalculateBounds();
    mesh.RecalculateNormals();  // Auto-compute normals from geometry

    return mesh;
}
```

Subclasses implement `Depth(float u, float v)` to define height fields—mountains, terrain, waves, etc.

---

### Cylinder

A **Cylinder** is a tube with circular cross-sections:

```
       ___
      /   \   <- top cap (optional)
     |     |
     |     |  <- side surface
     |     |
      \___/   <- bottom cap (optional)
```

Parameters:
- `segments`: Smoothness of circle (7 = heptagon, higher = rounder)
- `height`: Length of cylinder
- `radius`: Radius of circle
- `openEnded`: Whether to close the caps

### Placing Vertices on a Circle

To place vertices evenly around a circle, use trigonometry:

```csharp
for (int i = 0; i < segments; i++) {
    float ratio = (float)i / (segments - 1);  // [0, 1]
    float rad = ratio * Mathf.PI * 2;         // [0, 2π]

    float cos = Mathf.Cos(rad);
    float sin = Mathf.Sin(rad);
    float x = cos * radius;
    float z = sin * radius;
}
```

> **Why cos/sin?**
>
> On the unit circle:
> - `cos(θ)` gives the X coordinate
> - `sin(θ)` gives the Y coordinate (or Z in 3D)
>
> Multiply by radius to scale the circle.

### Sample: Cylinder.cs

**Generate cap vertices:**

```csharp
void GenerateCap(
    int segments, float top, float bottom, float radius,
    List<Vector3> vertices, List<Vector2> uvs,
    List<Vector3> normals, bool side)
{
    for (int i = 0; i < segments; i++) {
        float ratio = (float)i / (segments - 1);
        float rad = ratio * PI2;

        float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
        float x = cos * radius, z = sin * radius;

        Vector3 tp = new Vector3(x, top, z);
        Vector3 bp = new Vector3(x, bottom, z);

        vertices.Add(tp);  // Top ring vertex
        uvs.Add(new Vector2(ratio, 1f));

        vertices.Add(bp);  // Bottom ring vertex
        uvs.Add(new Vector2(ratio, 0f));

        if(side) {
            // Side normals point outward
            var normal = new Vector3(cos, 0f, sin);
            normals.Add(normal);
            normals.Add(normal);
        } else {
            // Cap normals point up/down
            normals.Add(new Vector3(0f, 1f, 0f));
            normals.Add(new Vector3(0f, -1f, 0f));
        }
    }
}
```

> **Why Separate Vertices for Caps?**
>
> Side surfaces and caps need different normals:
> - Sides: Normals point outward (radially)
> - Caps: Normals point up/down
>
> If vertices were shared, normals would be averaged, creating unnatural lighting at edges. Always duplicate vertices where you need different normals (hard edges).

**Build side triangles:**

```csharp
var len = (segments + 1) * 2;  // Total vertices in one ring pair

for (int i = 0; i < segments + 1; i++) {
    int idx = i * 2;
    int a = idx, b = idx + 1;
    int c = (idx + 2) % len, d = (idx + 3) % len;

    triangles.Add(a); triangles.Add(c); triangles.Add(b);
    triangles.Add(d); triangles.Add(b); triangles.Add(c);
}
```

**Build caps (pizza-slice triangles from center):**

```csharp
// Center vertices
vertices.Add(new Vector3(0f, top, 0f));     // Top center
vertices.Add(new Vector3(0f, bottom, 0f));  // Bottom center

var it = vertices.Count - 2;  // Top center index
var ib = vertices.Count - 1;  // Bottom center index

// Top cap triangles
for (int i = 0; i < len; i += 2) {
    triangles.Add(it);
    triangles.Add((i + 2) % len + offset);
    triangles.Add(i + offset);
}

// Bottom cap triangles
for (int i = 1; i < len; i += 2) {
    triangles.Add(ib);
    triangles.Add(i + offset);
    triangles.Add((i + 2) % len + offset);
}
```

---

### Tubular

A **Tubular** is a tube that follows a curve—like a pipe or branch:

Unlike a straight Cylinder, Tubular bends smoothly without twisting. This is essential for tree branches, tentacles, cables, and other organic shapes.

### Tubular Structure

1. **Divide the curve** into segments
2. **At each segment**, place a ring of vertices (like a Cylinder cross-section)
3. **Connect adjacent rings** with triangles

### Curves

The sample uses `CatmullRomCurve`, a spline that passes through all control points—intuitive for artists since the curve actually goes through the points you specify.

Key functions:
- `GetPointAt(float t)`: Position at parameter t ∈ [0,1]
- `GetTangentAt(float t)`: Direction at parameter t

### Frenet Frames

To create a twist-free tube along a curve, we need three orthogonal vectors at each point:

- **Tangent (T)**: Direction along the curve
- **Normal (N)**: Perpendicular to tangent
- **Binormal (B)**: Perpendicular to both (N × T)

These three vectors form a **Frenet frame**—a local coordinate system that moves along the curve.

> **Why Frenet Frames?**
>
> To place vertices in a circle around the curve, we need to know "which way is up" and "which way is sideways" at each point. The normal and binormal give us these directions.
>
> ```
> Circle vertex = CurvePoint + radius * (cos(θ) * N + sin(θ) * B)
> ```

### Sample: Tubular.cs

```csharp
// Get Frenet frames along curve
var frames = curve.ComputeFrenetFrames(tubularSegments, closed);

// Generate vertices for each segment
for(int i = 0; i < tubularSegments; i++) {
    GenerateSegment(curve, frames, vertices, normals, tangents, i);
}

// Generate triangles connecting adjacent rings
for (int j = 1; j <= tubularSegments; j++) {
    for (int i = 1; i <= radialSegments; i++) {
        int a = (radialSegments + 1) * (j - 1) + (i - 1);
        int b = (radialSegments + 1) * j + (i - 1);
        int c = (radialSegments + 1) * j + i;
        int d = (radialSegments + 1) * (j - 1) + i;

        triangles.Add(a); triangles.Add(d); triangles.Add(b);
        triangles.Add(b); triangles.Add(d); triangles.Add(c);
    }
}
```

**GenerateSegment function:**

```csharp
void GenerateSegment(CurveBase curve, List<FrenetFrame> frames,
    List<Vector3> vertices, List<Vector3> normals,
    List<Vector4> tangents, int index)
{
    var u = 1f * index / tubularSegments;
    var p = curve.GetPointAt(u);
    var fr = frames[index];

    var N = fr.Normal;
    var B = fr.Binormal;

    for(int j = 0; j <= radialSegments; j++) {
        float rad = 1f * j / radialSegments * PI2;

        float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
        var v = (cos * N + sin * B).normalized;

        vertices.Add(p + radius * v);
        normals.Add(v);
        tangents.Add(new Vector4(fr.Tangent.x, fr.Tangent.y, fr.Tangent.z, 0f));
    }
}
```

---

## Complex Shapes: Trees

Plants are a classic application of procedural modeling. Unity even includes a Tree Editor, and tools like SpeedTree specialize in this.

### L-Systems

**L-Systems** (Lindenmayer Systems) were proposed by botanist Aristid Lindenmayer in 1968 to describe plant structures.

L-Systems express **self-similarity**—the property where parts of an object resemble the whole at a smaller scale. Tree branches exhibit this: the branching pattern near the trunk is similar to branching patterns at the tips.

> **How L-Systems Work**
>
> 1. Start with an initial string (axiom)
> 2. Apply rewriting rules repeatedly
> 3. Each generation produces more complex results
>
> Example:
> - Axiom: `a`
> - Rules: `a → ab`, `b → a`
> - Generations: `a → ab → aba → abaab → abaababa → ...`

For graphics, symbols represent actions:
- **Draw**: Move forward, drawing a line
- **Turn Left**: Rotate left by θ degrees
- **Turn Right**: Rotate right by θ degrees

Applying rules recursively generates tree-like branching patterns—this self-similarity is also called **fractal** structure.

### ProceduralTree

The `ProceduralTree` class applies L-System concepts to generate 3D tree meshes.

Unlike the simple L-System example (fixed angles, binary branching), ProceduralTree uses randomness for:
- Number of branches at each fork
- Angles of branching
- Branch lengths and radii

### TreeData Class

`TreeData` contains parameters controlling tree shape:

**Branching parameters:**
- `branchesMin/branchesMax`: Range for number of child branches
- `growthAngleMin/growthAngleMax`: Range for branching angles
- `growthAngleScale`: Increases angle variation toward branch tips

**Mesh parameters:**
- `radialSegments`: Smoothness around branch circumference
- `heightSegments`: Segments along branch length

### TreeBranch Class

Each branch is a `TreeBranch` instance:

```csharp
var root = new TreeBranch(generations, length, radius, data);
```

The constructor recursively creates child branches:
- Each `TreeBranch` has a `List<TreeBranch> children`
- Starting from root, you can traverse the entire tree

### Branch Direction with Random Rotation

Each branch has tangent (direction), normal, and binormal vectors. When creating child branches:

```csharp
// More angle variation toward branch tips
var scale = Mathf.Lerp(1f, data.growthAngleScale,
                        1f - 1f * generation / generations);

// Rotation around normal axis
var qn = Quaternion.AngleAxis(scale * data.GetRandomGrowthAngle(), normal);

// Rotation around binormal axis
var qb = Quaternion.AngleAxis(scale * data.GetRandomGrowthAngle(), binormal);

// Apply rotations to get new branch direction
this.to = from + (qn * qb) * tangent * length;
```

### TreeSegment Class

Each branch is divided into segments (like Tubular):

```csharp
public class TreeSegment {
    public FrenetFrame Frame { get { return frame; } }
    public Vector3 Position { get { return position; } }
    public float Radius { get { return radius; } }

    FrenetFrame frame;    // Direction vectors
    Vector3 position;     // Segment center
    float radius;         // Segment width
}
```

### Building the Tree Mesh

The tree mesh combines all branches into one:

```csharp
var root = new TreeBranch(generations, length, radius, data);

// Calculate total tree length for UV mapping
float maxLength = TraverseMaxLength(root);

// Traverse all branches recursively
Traverse(root, (branch) => {
    var offset = vertices.Count;

    // Generate vertices for this branch
    for(int i = 0, n = branch.Segments.Count; i < n; i++) {
        var segment = branch.Segments[i];
        var N = segment.Frame.Normal;
        var B = segment.Frame.Binormal;

        for(int j = 0; j <= data.radialSegments; j++) {
            float rad = 1f * j / data.radialSegments * PI2;
            float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
            var normal = (cos * N + sin * B).normalized;

            vertices.Add(segment.Position + segment.Radius * normal);
            normals.Add(normal);
            // ... uvs, tangents
        }
    }

    // Generate triangles for this branch
    for (int j = 1; j <= data.heightSegments; j++) {
        for (int i = 1; i <= data.radialSegments; i++) {
            // ... quad indices with offset
        }
    }
});
```

> **Tree Modeling Goes Deeper**
>
> More sophisticated techniques consider:
> - Sunlight exposure (branches grow toward light)
> - Gravity effects (branches droop)
> - Wind response
>
> For further reading: "The Algorithmic Beauty of Plants" by Lindenmayer
> http://algorithmicbotany.org/papers/#abop

---

## Application: Teddy (Sketch-Based Modeling)

Procedural modeling isn't just for automated content—it enables interactive modeling tools.

**Teddy**, developed by Takeo Igarashi at the University of Tokyo, generates 3D models from 2D sketches:

1. User draws a 2D outline
2. Delaunay triangulation creates a 2D mesh
3. An inflation algorithm lifts the mesh into 3D

This technology powered "Rakugaki Oukoku" (2002, PlayStation 2), where players drew characters that became 3D fighters.

Teddy is available as a Unity asset: http://uniteddy.info/

> **The Power of Procedural Techniques**
>
> With procedural modeling, you can build custom modeling tools—enabling content that evolves through user creativity.

---

## Summary

Procedural modeling enables:

1. **Efficient model generation** with parametric control
2. **Interactive tools** that generate models from user input
3. **Runtime content** that adapts to gameplay

While this chapter focused on game/visualization applications, the techniques apply broadly:
- Architecture and product design (Grasshopper/Rhinoceros)
- Digital fabrication and 3D printing
- Scientific visualization

Understanding procedural modeling opens possibilities wherever shape needs to be computed rather than manually created.

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **Mesh = Vertices + Triangles**: Vertices are positions; triangles are triplets of vertex indices
>
> 2. **Winding order matters**: Clockwise = front-facing in Unity
>
> 3. **Duplicate vertices for hard edges**: Where normals differ, you need separate vertices
>
> 4. **Grid indexing**: `index = y * width + x` for 2D arrays in 1D storage
>
> 5. **Trigonometry for circles**: `(cos(θ) * r, sin(θ) * r)` places points on a circle
>
> 6. **Frenet frames**: Tangent + Normal + Binormal = local coordinate system along a curve
>
> 7. **L-Systems**: Recursive rewriting rules generate self-similar (fractal) structures
>
> 8. **RecalculateBounds()**: Always call after modifying vertices (needed for culling)
>
> 9. **RecalculateNormals()**: Auto-computes smooth normals from geometry

---

## Shape Hierarchy

| Shape | Structure | Key Concept |
|-------|-----------|-------------|
| **Quad** | 4 vertices, 2 triangles | Basic mesh building block |
| **Plane** | Grid of Quads | `y * width + x` indexing |
| **Cylinder** | Circular caps + side | Trigonometry for circles |
| **Tubular** | Cylinder along curve | Frenet frames |
| **Tree** | Recursive Tubulars | L-Systems, self-similarity |

---

## References

- The Algorithmic Beauty of Plants - http://algorithmicbotany.org/papers
- Teddy: A Sketching Interface for 3D Freeform Design - http://www-ui.is.s.u-tokyo.ac.jp/~takeo/papers/siggraph99.pdf
- Unity Mesh Documentation - https://docs.unity3d.com/Manual/AnatomyofaMesh.html

---

*Next chapter: MultiPlane Projection!*
