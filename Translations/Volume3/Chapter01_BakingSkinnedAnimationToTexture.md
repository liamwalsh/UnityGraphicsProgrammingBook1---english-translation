# Chapter 1: Baking Skinned Animation to Texture

**Author: Sugino (@sugi_cho)**

## Introduction

This chapter demonstrates how to display thousands or tens of thousands of skinned animated objects efficiently. The technique involves baking animation vertex data into textures, enabling GPU instancing for massive character rendering.

When you want to express flocks of birds or crowds in Unity, you typically use Animator and SkinnedMeshRenderer components. However, SkinnedMeshRenderer doesn't support GPU instancing, meaning each object must be rendered individually - resulting in extremely heavy processing.

The solution presented here stores animated vertex position information as textures. This chapter explains the methodology, implementation considerations, and applications.

## The Performance Problem

### Testing with 5000 Animated Objects

Using a simple horse model with 1,890 vertices and animation, rendering 5,000 horses with standard SkinnedMeshRenderer results in approximately 8.8 FPS - extremely poor performance.

### Profiler Analysis

Using Unity's Profiler (Window > Profiler or Ctrl+7), adding GPU Usage profiling reveals:
- GPU processing time exceeds CPU processing time
- CPU waits for GPU completion
- Approximately 70% of GPU processing is consumed by `PostLateUpdate.UpdateAllSkinnedMeshes`
- `Camera.Render` runs for each visible object

**Key Insight**: When GPU Skinning is enabled (Player Settings), the CPU calculates bone matrices and sends them to the GPU for skinning. With CPU skinning, both calculations happen on the CPU, resulting in even worse performance.

**Optimization Principle**: Always profile first to identify bottlenecks before optimizing.

## Pre-Computing Skinned Mesh Vertices

### Using SkinnedMeshRenderer.BakeMesh()

The `SkinnedMeshRenderer.BakeMesh(Mesh)` function creates a snapshot of the skinned mesh state and stores it in the specified mesh. While somewhat slow, it's suitable for pre-computing vertex information.

```csharp
Animator animator;
SkinnedMeshRenderer skinedMesh;
List<Mesh> meshList;

void Start(){
    animator = GetComponent<Animator>();
    skinnedMesh = GetComponentInChildren<SkinnedMeshRenderer>();
    meshList = new List<Mesh>();
    animator.Play("Run");
}

void Update(){
    var mesh = new Mesh();
    skinnedMesh.BakeMesh(mesh);
    // mesh now contains the skinned mesh snapshot
    meshList.Add(mesh);
}
```

### Why Store in Textures?

Storing full Mesh objects for each frame wastes memory on unchanging data (indices, UVs, etc.). For skinned animation, only vertex positions and normals change. Furthermore, updating mesh data per-frame via `Mesh.SetVertices()` creates significant CPU overhead.

The solution: Store position and normal data in textures and use **Vertex Texture Fetch (VTF)** in the vertex shader. This eliminates CPU overhead entirely.

## Writing Position Data to Textures

### Data Structure Mapping

| Vector3 (Position) | | Color (Texture) | |
|---|---|---|---|
| x | float | r | float |
| y | float | g | float |
| z | float | b | float |

### Implementation

```csharp
public void CreateTex(Mesh sourceMesh)
{
    var vertCount = sourceMesh.vertexCount;
    var width = Mathf.FloorToInt(Mathf.Sqrt(vertCount));
    var height = Mathf.CeilToInt((float)vertCount / width);
    // Ensure vertCount < width * height

    // RGBAFloat format preserves full float precision
    posTex = new Texture2D(width, height, TextureFormat.RGBAFloat, false);
    normTex = new Texture2D(width, height, TextureFormat.RGBAFloat, false);

    var vertices = sourceMesh.vertices;
    var normals = sourceMesh.normals;
    var posColors = new Color[width * height];
    var normColors = new Color[width * height];

    for (var i = 0; i < vertCount; i++)
    {
        posColors[i] = new Color(
            vertices[i].x,
            vertices[i].y,
            vertices[i].z
        );
        normColors[i] = new Color(
            normals[i].x,
            normals[i].y,
            normals[i].z
        );
    }

    posTex.SetPixels(posColors);
    normTex.SetPixels(normColors);
    posTex.Apply();
    normTex.Apply();
}
```

**Note**: Although Unity documentation states `Texture2D.SetPixels()` only works with fixed-point formats (RGBA32, ARGB32, RGB24, Alpha8), it actually works with RGBAHalf and RGBAFloat, preserving negative values and values greater than 1 without clamping.

## Complete Implementation

The system consists of three components:

1. **AnimationClipTextureBaker.cs** - Samples animation and creates ComputeBuffers
2. **MeshInfoTextureGen.compute** - Converts buffers to textures via ComputeShader
3. **TextureAnimPlayer.shader** - Plays animation from textures

### Vertex Information Structure

```csharp
public struct VertInfo
{
    public Vector3 position;
    public Vector3 normal;
}
```

### Animation Sampling with AnimationClip.SampleAnimation()

```csharp
// Sample animation at specific time without Animator component
clip.SampleAnimation(gameObject, time);
```

### ComputeShader for Texture Generation

```hlsl
#pragma kernel CSMain

struct MeshInfo{
    float3 position;
    float3 normal;
};

RWTexture2D<float4> OutPosition;
RWTexture2D<float4> OutNormal;
StructuredBuffer<MeshInfo> Info;
int VertCount;

[numthreads(8,8,1)]
void CSMain (uint3 id : SV_DispatchThreadID)
{
    int index = id.y * VertCount + id.x;
    MeshInfo info = Info[index];

    OutPosition[id.xy] = float4(info.position, 1.0);
    OutNormal[id.xy] = float4(info.normal, 1.0);
    // X-axis = vertex ID, Y-axis = time
}
```

### Texture Configuration

**Critical Settings**:
- `FilterMode = Bilinear` - Enables automatic interpolation between frames
- `WrapMode = Repeat` - For looping animations (seamless loop)
- `WrapMode = Clamp` - For non-looping animations

Bilinear filtering automatically interpolates between adjacent pixels, creating smooth animation even between sampled keyframes.

### Animation Playback Shader

```hlsl
Shader "Unlit/TextureAnimPlayer"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _PosTex("position texture", 2D) = "black"{}
        _NmlTex("normal texture", 2D) = "white"{}
        _DT ("delta time", float) = 0
        _Length ("animation length", Float) = 1
        [Toggle(ANIM_LOOP)] _Loop("loop", Float) = 0
    }

    SubShader
    {
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile ___ ANIM_LOOP

            #include "UnityCG.cginc"
            #define ts _PosTex_TexelSize

            struct appdata
            {
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                float3 normal : TEXCOORD1;
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex, _PosTex, _NmlTex;
            float4 _PosTex_TexelSize;
            float _Length, _DT;

            v2f vert (appdata v, uint vid : SV_VertexID)
            {
                float t = (_Time.y - _DT) / _Length;
                #if ANIM_LOOP
                    t = fmod(t, 1.0);
                #else
                    t = saturate(t);
                #endif

                // UV.x = vertex ID, UV.y = normalized time
                float x = (vid + 0.5) * ts.x;
                float y = t;

                float4 pos = tex2Dlod(_PosTex, float4(x, y, 0, 0));
                float3 normal = tex2Dlod(_NmlTex, float4(x, y, 0, 0));

                v2f o;
                o.vertex = UnityObjectToClipPos(pos);
                o.normal = UnityObjectToWorldNormal(normal);
                o.uv = v.uv;
                return o;
            }

            half4 frag (v2f i) : SV_Target
            {
                half diff = dot(i.normal, float3(0, 1, 0)) * 0.5 + 0.5;
                half4 col = tex2D(_MainTex, i.uv);
                return diff * col;
            }
            ENDCG
        }
    }
}
```

**Key Concepts**:
- `SV_VertexID` semantic retrieves the vertex index
- The `+0.5` offset ensures sampling at pixel centers (avoiding interpolation between vertices)
- `_TexelSize` contains texture dimensions: x=1/width, y=1/height, z=width, w=height

## Results

With texture-based animation, 5,000 horses render at **56.4 FPS** compared to 8.8 FPS with SkinnedMeshRenderer - a **6.4x improvement** without any additional optimizations like GPU instancing.

Since SkinnedMeshRenderer is no longer used, GPU instancing becomes possible, enabling even further performance gains with `Graphics.DrawMeshInstancedIndirect()`.

## Limitations and Considerations

### Constraints

1. **Memory Usage**: Texture size depends on vertex count and animation length
2. **Animation Blending**: Requires custom shader implementation
3. **State Machine**: Cannot use AnimatorController
4. **Maximum Texture Size**: Hardware-dependent (4K, 8K, or 16K)

### Texture Size Limitation Workarounds

- Use multiple textures for high vertex counts
- Bake skeleton bone matrices instead of vertex positions (reduces data, enables runtime skinning in vertex shader)

### Best Use Cases

This technique works best for:
- Looping animations on background characters
- Flocks of birds or butterflies
- Crowd simulations
- Any scenario where precise animation control is less critical

## Key Takeaways

1. **Always profile first** - Identify bottlenecks before optimizing
2. **Skinned animation is expensive** - `UpdateAllSkinnedMeshes` often dominates GPU time
3. **Pre-baking to textures** eliminates runtime skinning cost
4. **GPU instancing becomes possible** when not using SkinnedMeshRenderer
5. **The technique generalizes** - Skeleton matrices, simulation results, or any expensive computation can be pre-baked to textures

## Further Reading

- GPU Instancing documentation
- Vertex Texture Fetch (VTF) techniques
- Author's GitHub for advanced examples including instanced rendering
