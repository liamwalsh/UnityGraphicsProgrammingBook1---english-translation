# Chapter 7: Introduction to Marching Cubes

*Original author: @kaiware007*
*Translation and annotations by Claude*

---

## What is Marching Cubes?

### History and Overview

**Marching Cubes** is a volume rendering algorithm that converts 3D voxel data (filled with scalar values) into polygon mesh data. It was first published by William E. Lorensen and Harvey E. Cline in 1987.

The algorithm was patented, but the patent expired in 2005, so it's now freely usable.

> **Where You've Seen It**
>
> Marching Cubes is used in:
> - Medical imaging (CT/MRI visualization)
> - Terrain generation with destructible voxels (games like Astroneer)
> - Fluid surface extraction
> - 3D scanning/reconstruction
> - Metaball rendering

### How It Works (Simplified)

**Step 1:** Divide the volume data space into a 3D grid.

**Step 2:** For each grid cell (cube), examine the 8 corner values. If a corner's value is above the threshold, mark it as 1; otherwise 0.

**Step 3:** The 8 corners give 2^8 = 256 possible combinations. Through rotation and reflection symmetry, these reduce to just **15 unique patterns**.

**Step 4:** Look up the triangle pattern for this combination and generate the corresponding polygons.

> **Why "Marching"?**
>
> The algorithm "marches" through the volume, processing one cube at a time, hence the name. Each cube is processed independently, making it highly parallelizable on GPUs.

---

## Sample Repository

The sample project is in **Assets/GPUMarchingCubes** at:
https://github.com/IndieVisualLab/UnityGraphicsProgramming

Implementation is based on Paul Bourke's "Polygonising a scalar field" article.

The implementation has three main parts:
1. Mesh initialization and per-frame rendering (C# scripts)
2. ComputeBuffer initialization
3. Actual rendering (shaders)

---

## Creating Geometry Shader-Compatible Meshes

As explained, Marching Cubes generates polygons based on the 8 corners of each grid cell. For real-time rendering, we need dynamic polygon generation.

Creating mesh vertex arrays on the CPU every frame would be inefficient. Instead, we use **Geometry Shaders**—they sit between Vertex and Fragment shaders and can dynamically create/modify vertices on the GPU, making them very fast.

### Variable Definitions

```csharp
public class GPUMarchingCubesDrawMesh : MonoBehaviour {

    #region public
    public int segmentNum = 32;              // Grid divisions per axis
    [Range(0,1)]
    public float threashold = 0.5f;          // Threshold for mesh generation
    public Material mat;                     // Rendering material

    public Color DiffuseColor = Color.green;
    public Color EmissionColor = Color.black;
    public float EmissionIntensity = 0;

    [Range(0,1)]
    public float metallic = 0;
    [Range(0, 1)]
    public float glossiness = 0.5f;
    #endregion

    #region private
    int vertexMax = 0;                       // Total vertex count
    Mesh[] meshs = null;                     // Mesh array
    Material[] materials = null;             // Per-mesh materials
    float renderScale = 1f / 32f;            // Display scale
    MarchingCubesDefines mcDefines = null;   // MC lookup tables
    #endregion
}
```

### Mesh Creation

Vertices are placed one per grid cell. For 64 divisions per axis: 64×64×64 = 262,144 vertices needed.

However, Unity (2017.1.1f1) limits meshes to 65,535 vertices. So we split into multiple meshes:

```csharp
void CreateMesh()
{
    // Split meshes to stay under 65535 vertex limit
    int vertNum = 65535;
    int meshNum = Mathf.CeilToInt((float)vertexMax / vertNum);

    meshs = new Mesh[meshNum];
    materials = new Material[meshNum];

    // Calculate bounds
    Bounds bounds = new Bounds(
        transform.position,
        new Vector3(segmentNum, segmentNum, segmentNum) * renderScale
    );

    int id = 0;
    for (int i = 0; i < meshNum; i++)
    {
        Vector3[] vertices = new Vector3[vertNum];
        int[] indices = new int[vertNum];

        for(int j = 0; j < vertNum; j++)
        {
            // Encode 3D grid position in vertex coordinates
            vertices[j].x = id % segmentNum;
            vertices[j].y = (id / segmentNum) % segmentNum;
            vertices[j].z = (id / (segmentNum * segmentNum)) % segmentNum;

            indices[j] = j;
            id++;
        }

        meshs[i] = new Mesh();
        meshs[i].vertices = vertices;
        // Use Points topology - Geometry Shader will create actual polygons
        meshs[i].SetIndices(indices, MeshTopology.Points, 0);
        meshs[i].bounds = bounds;

        materials[i] = new Material(mat);
    }
}
```

> **Key Insight**
>
> Each vertex represents a grid cell position. The Geometry Shader will read the scalar field at each cell's 8 corners and generate triangles accordingly. Using `MeshTopology.Points` means we're just passing positions—the shader does the rest.

---

## ComputeBuffer Initialization

The `MarchingCubesDefines.cs` file contains lookup tables and ComputeBuffers for the Marching Cubes algorithm.

**Why ComputeBuffers instead of shader constants?**

Shaders have a limit of ~4096 literal values. The Marching Cubes lookup tables are huge and would exceed this limit. By storing them in ComputeBuffers (GPU memory), we avoid this restriction and get fast shader access.

---

## Rendering

```csharp
void RenderMesh()
{
    Vector3 halfSize = new Vector3(segmentNum, segmentNum, segmentNum)
                       * renderScale * 0.5f;
    Matrix4x4 trs = Matrix4x4.TRS(
                       transform.position,
                       transform.rotation,
                       transform.localScale
                    );

    for (int i = 0; i < meshs.Length; i++)
    {
        materials[i].SetPass(0);
        materials[i].SetInt("_SegmentNum", segmentNum);
        materials[i].SetFloat("_Scale", renderScale);
        materials[i].SetFloat("_Threashold", threashold);
        // ... (other parameters)

        Graphics.DrawMesh(meshs[i], Matrix4x4.identity, materials[i], 0);
    }
}
```

We use `Graphics.DrawMesh()` for Unity lighting integration. This registers the mesh for rendering (doesn't draw immediately), allowing Unity to apply lights and shadows.

> **DrawMesh vs DrawMeshNow**
>
> - `DrawMesh()`: Registers for rendering, Unity handles lighting/shadows, call in Update()
> - `DrawMeshNow()`: Immediate draw, no Unity lighting, call in OnRenderObject()/OnPostRender()

---

## Shader Implementation

The shader has two main parts: **object rendering** and **shadow rendering**. Each uses vertex, geometry, and fragment shaders.

### Data Structures

```hlsl
// Mesh input data
struct appdata
{
    float4 vertex : POSITION;
};

// Vertex → Geometry
struct v2g
{
    float4 pos : SV_POSITION;
};

// Geometry → Fragment (object rendering)
struct g2f_light
{
    float4 pos      : SV_POSITION;  // Clip space position
    float3 normal   : NORMAL;
    float4 worldPos : TEXCOORD0;
    half3 sh        : TEXCOORD3;    // Spherical harmonics
};

// Geometry → Fragment (shadow rendering)
struct g2f_shadow
{
    float4 pos  : SV_POSITION;
    float4 hpos : TEXCOORD1;
};
```

### Shader Variables

```hlsl
int _SegmentNum;
float _Scale;
float _Threashold;
float4 _DiffuseColor;
// ... (other parameters)

// Lookup tables from ComputeBuffers
StructuredBuffer<float3> vertexOffset;
StructuredBuffer<int> cubeEdgeFlags;
StructuredBuffer<int2> edgeConnection;
StructuredBuffer<float3> edgeDirection;
StructuredBuffer<int> triangleConnectionTable;
```

### Vertex Shader

Very simple—just passes vertex data to geometry shader:

```hlsl
v2g vert(appdata v)
{
    v2g o = (v2g)0;
    o.pos = v.vertex;  // Grid position encoded in vertex
    return o;
}
```

### Geometry Shader (Object)

```hlsl
[maxvertexcount(15)]  // Max 5 triangles × 3 vertices = 15
void geom_light(point v2g input[1],
                inout TriangleStream<g2f_light> outStream)
{
    float3 pos = input[0].pos.xyz;
    float cubeValue[8];

    // Sample scalar field at 8 cube corners
    for (int i = 0; i < 8; i++) {
        cubeValue[i] = Sample(
            pos.x + vertexOffset[i].x,
            pos.y + vertexOffset[i].y,
            pos.z + vertexOffset[i].z
        );
    }

    // Build flag index from corner values vs threshold
    int flagIndex = 0;
    for (i = 0; i < 8; i++) {
        if (cubeValue[i] <= _Threashold) {
            flagIndex |= (1 << i);
        }
    }

    int edgeFlags = cubeEdgeFlags[flagIndex];

    // Empty or full cube - no surface
    if ((edgeFlags == 0) || (edgeFlags == 255)) {
        return;
    }

    // Calculate edge vertices where surface intersects cube edges
    float3 edgeVertices[12];
    float3 edgeNormals[12];

    for (i = 0; i < 12; i++) {
        if ((edgeFlags & (1 << i)) != 0) {
            // Interpolate along edge based on threshold
            float offset = getOffset(
                cubeValue[edgeConnection[i].x],
                cubeValue[edgeConnection[i].y],
                _Threashold
            );

            float3 vertex = vertexOffset[edgeConnection[i].x]
                          + offset * edgeDirection[i];

            edgeVertices[i] = pos + vertex * _Scale;
            edgeNormals[i] = getNormal(pos + vertex);
        }
    }

    // Generate triangles from lookup table
    for (i = 0; i < 5; i++) {  // Max 5 triangles
        int findex = flagIndex * 16 + 3 * i;
        if (triangleConnectionTable[findex] < 0)
            break;

        for (int j = 0; j < 3; j++) {
            int vindex = triangleConnectionTable[findex + j];

            g2f_light o;
            float4 ppos = mul(_Matrix, float4(edgeVertices[vindex], 1));
            o.pos = UnityObjectToClipPos(ppos);
            o.normal = normalize(mul(_Matrix, float4(edgeNormals[vindex], 0)));
            o.worldPos = ppos;

            outStream.Append(o);
        }
        outStream.RestartStrip();
    }
}
```

> **Understanding the Algorithm**
>
> 1. Sample 8 corners → 8 scalar values
> 2. Compare to threshold → 8-bit flag (256 possibilities)
> 3. Look up edge flags → which of 12 edges are crossed
> 4. For each crossed edge, interpolate vertex position
> 5. Look up triangle table → which edge vertices connect
> 6. Output triangles

---

## Distance Functions

Instead of sampling from actual volume data, this example uses **distance functions** to define shapes procedurally.

```hlsl
// Sphere distance function
inline float sphere(float3 pos, float radius)
{
    return length(pos) - radius;
}
```

If `sphere()` returns negative, the point is inside the sphere. This simple formula defines complex 3D shapes with minimal code.

```hlsl
// Smooth blending of distance functions (metaball effect)
float smoothMax(float d1, float d2, float k)
{
    float h = exp(k * d1) + exp(k * d2);
    return log(h) / k;
}
```

The `smoothMax` function blends multiple shapes smoothly, creating metaball-like organic forms.

> **Distance Function Resources**
>
> Inigo Quilez's website has an excellent collection:
> http://iquilezles.org/www/articles/distfunctions/distfunctions.htm

---

## Fragment Shader (Object)

Uses Unity's Standard lighting with deferred rendering:

```hlsl
void frag_light(g2f_light IN,
    out half4 outDiffuse        : SV_Target0,
    out half4 outSpecSmoothness : SV_Target1,
    out half4 outNormal         : SV_Target2,
    out half4 outEmission       : SV_Target3)
{
    // Initialize surface output
    SurfaceOutputStandard o;
    o.Albedo = _DiffuseColor.rgb;
    o.Emission = _EmissionColor * _EmissionIntensity;
    o.Metallic = _Metallic;
    o.Smoothness = _Glossiness;
    o.Normal = IN.normal;
    // ...

    // Setup GI
    UnityGI gi;
    UnityGIInput giInput;
    // ... (GI setup code)

    LightingStandard_GI(o, giInput, gi);

    // Output to G-buffer
    outEmission = LightingStandard_Deferred(o, worldViewDir, gi,
                                            outDiffuse,
                                            outSpecSmoothness,
                                            outNormal);
}
```

This integrates with Unity's deferred rendering for proper lighting, reflections, and GI.

---

## Shadow Shaders

The shadow geometry shader is nearly identical to the object version, but outputs to shadow maps:

```hlsl
// Shadow vertex transformation
o.pos = UnityClipSpaceShadowCasterPos(lpos, normal);
o.pos = UnityApplyLinearShadowBias(o.pos);
```

The shadow fragment shader is trivial—Unity handles the heavy lifting:

```hlsl
fixed4 frag_shadow(g2f_shadow i) : SV_Target
{
    return i.hpos.z / i.hpos.w;
}
```

---

## Results

Running the sample produces animated metaball-like shapes:

The distance functions can be combined to create various forms—the sample shows animated spheres that merge organically.

---

## Summary

This chapter used distance functions for simplicity, but Marching Cubes works with any 3D scalar data:
- 3D textures with volume data
- Medical imaging data (CT/MRI)
- Procedural terrain

For games, you could create destructible/constructible terrain like Astroneer by modifying the underlying volume data.

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **Marching Cubes**: Converts volume data to polygons by examining cube corners
>
> 2. **256 → 15 patterns**: Symmetry reduces lookup table complexity
>
> 3. **Geometry Shader**: Creates triangles dynamically on GPU
>
> 4. **ComputeBuffers**: Store large lookup tables (bypass shader literal limits)
>
> 5. **Distance functions**: Define shapes procedurally with simple math
>
> 6. **Edge interpolation**: Place vertices where surface crosses cube edges
>
> 7. **MeshTopology.Points**: Pass grid positions, geometry shader generates actual geometry

---

## The 15 Marching Cubes Cases

| Case | Description |
|------|-------------|
| 0 | All corners outside (empty) |
| 1 | One corner inside (single triangle) |
| 2 | Two adjacent corners (quad) |
| 3 | Two diagonal corners |
| 4 | Three corners (various configs) |
| ... | ... |
| 14 | Seven corners inside |
| 15 | All corners inside (full, no surface) |

Each case has a predefined triangle configuration stored in the lookup table.

---

## References

- Polygonising a scalar field - http://paulbourke.net/geometry/polygonise/
- Distance functions - http://iquilezles.org/www/articles/distfunctions/distfunctions.htm

---

*Next chapter: MCMC 3D Sampling!*
