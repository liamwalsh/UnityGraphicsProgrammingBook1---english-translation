# Chapter 3: Geometry Shader Applications for Line Expressions

**Author**: kaiware007

**Sample Project**: "GeometryWireframe" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter explores using Geometry Shaders to render wireframe polygons - a technique used in the author's visual artwork for Art Hack Day 2018. The approach demonstrates how to dynamically generate vertices in the GPU pipeline to create line-based visual effects.

---

## Part 1: Basic Line Drawing with Graphics.DrawProcedural

### Understanding the Foundation

In Unity, lines are typically drawn using `LineRenderer` or the `GL` class. However, when expecting high draw volumes, `Graphics.DrawProcedural` offers better performance by leveraging GPU-based vertex generation.

### Simple Sine Wave Example

The sample scene "SampleWaveLine" demonstrates basic line drawing:

```csharp
[ExecuteInEditMode]
public class RenderWaveLine : MonoBehaviour {
    [Range(2,50)]
    public int vertexNum = 4;
    public Material material;

    private void OnRenderObject()
    {
        material.SetInt("_VertexNum", vertexNum - 1);
        material.SetPass(0);
        Graphics.DrawProcedural(MeshTopology.LineStrip, vertexNum);
    }
}
```

**Key Points**:
- `Graphics.DrawProcedural` executes immediately, so it must be called within `OnRenderObject`
- `OnRenderObject` is called after all cameras finish rendering the scene
- The first argument is `MeshTopology` - specifying how mesh vertices are connected
- Available topologies: `Triangles`, `Quads`, `Lines`, `LineStrip`, `Points`

### Shader-Based Vertex Generation

The clever aspect of this approach is that vertex positions are calculated entirely in the shader:

```hlsl
v2f vert (uint id : SV_VertexID)
{
    float div = (float)id / _VertexNum;
    float4 pos = float4(
        (div - 0.5) * _ScaleX,
        sin(div * 2 * PI + _Time.y * _Speed) * _ScaleY,
        0, 1);

    v2f o;
    o.vertex = UnityObjectToClipPos(pos);
    return o;
}
```

**How It Works**:
- `SV_VertexID` provides a unique vertex index (0 to vertexCount-1)
- Dividing by vertex count gives a 0-1 ratio
- This ratio drives the sine wave calculation
- No vertex data needs to be passed from C# - it's all computed on the GPU

---

## Part 2: Drawing 2D Polygons with Geometry Shaders

### The Geometry Shader Concept

Geometry Shaders sit between Vertex and Fragment shaders, with the unique ability to **generate new vertices**. From a single input point, we can create an entire polygon.

### Implementation

The C# side is minimal:

```csharp
[ExecuteInEditMode]
public class SinglePolygon2D : MonoBehaviour {
    [Range(2, 64)]
    public int vertexNum = 3;
    public Material material;

    private void OnRenderObject()
    {
        material.SetInt("_VertexNum", vertexNum);
        material.SetMatrix("_TRS", transform.localToWorldMatrix);
        material.SetPass(0);
        Graphics.DrawProcedural(MeshTopology.Points, 1);
    }
}
```

**Notable Changes**:
- `MeshTopology.Points` is used (only 1 vertex required)
- `_TRS` matrix enables transform control via the GameObject

### The Geometry Shader

```hlsl
#pragma geometry geom

[maxvertexcount(65)]
void geom(point Output input[1], inout LineStream<Output> outStream)
{
    Output o;
    float rad = 2.0 * PI / (float)_VertexNum;
    float time = _Time.y * _Speed;
    float4 pos;

    for (int i = 0; i <= _VertexNum; i++) {
        pos.x = cos(i * rad + time) * _Scale;
        pos.y = sin(i * rad + time) * _Scale;
        pos.z = 0;
        pos.w = 1;
        o.pos = UnityObjectToClipPos(pos);
        outStream.Append(o);
    }
    outStream.RestartStrip();
}
```

**Key Attributes**:

| Attribute | Description |
|-----------|-------------|
| `[maxvertexcount(65)]` | Maximum vertices the shader can output (64 polygon vertices + 1 to close) |
| `point Output input[1]` | Input from vertex shader - `point` means 1 vertex, use `triangle` for mesh work |
| `inout LineStream<Output>` | Output type - also available: `PointStream`, `TriangleStream` |

**Stream Operations**:
- `Append()` - adds a vertex to the current line strip
- `RestartStrip()` - ends current strip, next `Append()` starts a new disconnected line

---

## Part 3: Creating an Octahedron Sphere

### What is an Octahedron Sphere?

A regular octahedron consists of 8 equilateral triangles. An Octahedron Sphere is created by subdividing these triangles using **spherical linear interpolation (slerp)**, causing vertices to curve outward into a sphere shape.

Unlike linear interpolation (straight line between points), spherical interpolation follows an arc along a sphere's surface.

### The Subdivision Algorithm

The algorithm works on each of the 8 triangular faces:

1. **Define Initial Vertices**: Store the 24 vertices of the normalized octahedron (8 triangles x 3 vertices)

2. **Iterate Subdivision Levels**: For each level n:
   - Divide each edge into n segments using slerp
   - Create new triangles from the subdivided points

3. **Spherical Linear Interpolation (Slerp)**:

```hlsl
float4 qslerp(float4 a, float4 b, float t)
{
    float4 r;
    float t_ = 1 - t;
    float wa, wb;
    float theta = acos(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
    float sn = sin(theta);
    wa = sin(t_ * theta) / sn;
    wb = sin(t * theta) / sn;
    r.x = wa * a.x + wb * b.x;
    r.y = wa * a.y + wb * b.y;
    r.z = wa * a.z + wb * b.z;
    r.w = wa * a.w + wb * b.w;
    normalize(r);
    return r;
}
```

### Triangle Subdivision Flow (n=2 Example)

**Step 1**: Calculate edge points using slerp ratios

```hlsl
float4 edge_p1 = qslerp(init_vectors[i], init_vectors[i + 2], (float)p / n);
float4 edge_p2 = qslerp(init_vectors[i + 1], init_vectors[i + 2], (float)p / n);
float4 edge_p3 = qslerp(init_vectors[i], init_vectors[i + 2], (float)(p + 1) / n);
float4 edge_p4 = qslerp(init_vectors[i + 1], init_vectors[i + 2], (float)(p + 1) / n);
```

**Step 2**: Calculate intermediate vertices (a, b, c, d) by interpolating between edge points

**Step 3**: Output triangles (a,b,c) and (c,b,d) using the stream

---

## Bonus Samples

The repository includes additional advanced examples:

1. **GPU Instancing Version** - SampleOctahedronSphereInstancing scene
2. **High Subdivision (9 levels)** - SampleOctahedronSphereMultiVertex scene
3. **High Subdivision + Instancing** - SampleOctahedronSphereMultiVertexInstancing scene

---

## Key Takeaways

1. **Geometry Shaders enable dynamic vertex generation** - Create complex geometry from minimal input data

2. **`Graphics.DrawProcedural` bypasses mesh overhead** - Ideal for procedural line/point rendering

3. **`SV_VertexID` enables shader-only vertex calculation** - No CPU-GPU data transfer needed

4. **Stream primitives (`LineStream`, `TriangleStream`)** control output geometry type

5. **Spherical interpolation creates smooth spherical surfaces** - Essential for geodesic sphere generation

6. **Geometry Shaders have vertex limits** - Plan `maxvertexcount` carefully for complex geometry

---

## Applications

While Geometry Shaders are commonly used for:
- Polygon subdivision
- Particle billboard generation

The techniques in this chapter open possibilities for:
- Dynamic wireframe visualizations
- Procedural geometric patterns
- Real-time generative art
- Efficient line-based visual effects

Experiment with these foundations to discover new visual expressions!
