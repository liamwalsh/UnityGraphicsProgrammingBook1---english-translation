# Chapter 6: Tessellation & Displacement

**Author:** Sakota
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** Tessellation

---

## Introduction

This chapter explains **Tessellation** - the GPU feature for subdividing polygons - and how to use **Displacement Maps** to offset the resulting vertices.

### Requirements

- ComputeShader support: Shader Model 5.0+
- Tessellation Shader only: Shader Model 4.6+
- Tested with Unity 2018.3.9

---

## What is Tessellation?

**Tessellation** is a standard feature in modern rendering pipelines (DirectX, OpenGL, Metal) that subdivides polygons on the GPU.

### Why Use Tessellation?

Traditionally, all vertex data is transferred from CPU to GPU. For high-polygon meshes, this CPU-GPU transfer becomes a bottleneck. Tessellation solves this by:

1. Processing a reduced-polygon mesh on the CPU
2. Subdividing it on the GPU
3. Using displacement maps to restore fine detail

This reduces bandwidth while maintaining visual quality.

---

## Tessellation Pipeline Stages

Tessellation adds three stages to the rendering pipeline:

1. **Hull Shader** - Programmable: Defines subdivision method and level
2. **Tessellator** - Fixed-function: Performs the actual subdivision
3. **Domain Shader** - Programmable: Positions the new vertices

The pipeline flow becomes:
```
Vertex Shader -> Hull Shader -> Tessellator -> Domain Shader -> [Geometry Shader] -> Fragment Shader
```

---

## Surface Shader Implementation

Unity provides convenient tessellation wrappers for Surface Shaders. This is the easiest way to get started.

```hlsl
Shader "Custom/TessellationDisplacement"
{
    Properties
    {
        _EdgeLength ("Edge length", Range(2,50)) = 15
        _MainTex ("Base (RGB)", 2D) = "white" {}
        _DispTex ("Disp Texture", 2D) = "black" {}
        _NormalMap ("Normalmap", 2D) = "bump" {}
        _Displacement ("Displacement", Range(0, 1.0)) = 0.3
        _Color ("Color", color) = (1,1,1,0)
        _SpecColor ("Spec color", color) = (0.5,0.5,0.5,0.5)
        _Specular ("Specular", Range(0, 1)) = 0
        _Gloss ("Gloss", Range(0, 1)) = 0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 300

        CGPROGRAM
        // tessellate: specifies the tessellation function
        // vertex: specifies the displacement function (called in Domain Shader)
        #pragma surface surf BlinnPhong addshadow fullforwardshadows \
            vertex:disp tessellate:tessEdge nolightmap
        #pragma target 4.6
        #include "Tessellation.cginc"

        struct appdata
        {
            float4 vertex : POSITION;
            float4 tangent : TANGENT;
            float3 normal : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        sampler2D _DispTex;
        float _Displacement;
        float _EdgeLength;

        // Tessellation function - called per patch, not per vertex
        // Returns subdivision levels: xyz = edge factors, w = inside factor
        float4 tessEdge(appdata v0, appdata v1, appdata v2)
        {
            // Tessellation.cginc provides helper functions:
            // - UnityDistanceBasedTess: Based on camera distance
            // - UnityEdgeLengthBasedTess: Based on edge length
            // - UnityEdgeLengthBasedTessCull: Edge length + frustum culling

            return UnityEdgeLengthBasedTessCull(
                v0.vertex, v1.vertex, v2.vertex,
                _EdgeLength, _Displacement * 1.5f
            );
        }

        // Displacement function - called in Domain Shader after tessellation
        void disp(inout appdata v)
        {
            // Sample displacement map and offset along normal
            float d = tex2Dlod(
                _DispTex,
                float4(v.texcoord.xy, 0, 0)
            ).r * _Displacement;
            v.vertex.xyz += v.normal * d;
        }

        struct Input
        {
            float2 uv_MainTex;
        };

        sampler2D _MainTex;
        sampler2D _NormalMap;
        fixed4 _Color;
        float _Specular;
        float _Gloss;

        void surf(Input IN, inout SurfaceOutput o)
        {
            half4 c = tex2D(_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            o.Specular = _Specular;
            o.Gloss = _Gloss;
            o.Normal = UnpackNormal(tex2D(_NormalMap, IN.uv_MainTex));
        }
        ENDCG
    }
    FallBack "Diffuse"
}
```

### Built-in Tessellation Functions

Unity's `Tessellation.cginc` provides:

| Function | Description |
|----------|-------------|
| `UnityDistanceBasedTess` | More subdivision when closer to camera |
| `UnityEdgeLengthBasedTess` | Subdivision based on screen-space edge length |
| `UnityEdgeLengthBasedTessCull` | Edge-based + frustum culling optimization |

---

## Vertex/Fragment Shader Implementation

For more control, you can implement tessellation manually in vertex/fragment shaders.

### Hull Shader

The Hull Shader runs after the Vertex Shader. It consists of two functions:
- **Control Point Function** - Runs per control point (vertex)
- **Patch Constant Function** - Runs per patch (primitive)

```hlsl
#pragma hull hull_shader

// Input structure for hull shader
struct InternalTessInterp_appdata
{
    float4 vertex : INTERNALTESSPOS;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

// Tessellation factors returned by patch constant function
struct TessellationFactors
{
    float edge[3] : SV_TessFactor;      // Edge subdivision levels
    float inside : SV_InsideTessFactor;  // Interior subdivision level
};

// Patch Constant Function - runs once per patch
TessellationFactors hull_const(InputPatch<InternalTessInterp_appdata, 3> v)
{
    TessellationFactors o;
    float4 tf;

    tf = UnityEdgeLengthBasedTessCull(
        v[0].vertex, v[1].vertex, v[2].vertex,
        _EdgeLength, _Displacement * 1.5f
    );

    o.edge[0] = tf.x;
    o.edge[1] = tf.y;
    o.edge[2] = tf.z;
    o.inside = tf.w;
    return o;
}

// Control Point Function - runs once per control point
[UNITY_domain("tri")]                    // Triangle primitives
[UNITY_partitioning("fractional_odd")]   // Smooth subdivision
[UNITY_outputtopology("triangle_cw")]    // Clockwise winding
[UNITY_patchconstantfunc("hull_const")]  // Patch constant function name
[UNITY_outputcontrolpoints(3)]           // 3 control points per patch
InternalTessInterp_appdata hull_shader(
    InputPatch<InternalTessInterp_appdata, 3> v,
    uint id : SV_OutputControlPointID
)
{
    return v[id];
}
```

### Hull Shader Attributes

| Attribute | Options | Description |
|-----------|---------|-------------|
| `UNITY_domain` | "tri", "quad", "isoline" | Primitive type |
| `UNITY_partitioning` | "integer", "fractional_odd", "fractional_even" | Subdivision interpolation |
| `UNITY_outputtopology` | "triangle_cw", "triangle_ccw", "line" | Output winding/type |
| `UNITY_patchconstantfunc` | function name | Patch constant function |
| `UNITY_outputcontrolpoints` | number | Control points per patch |

### Tessellator Stage

This is the fixed-function stage. It reads the tessellation factors from the Hull Shader and subdivides the patch accordingly. No code is written for this stage.

### Domain Shader

The Domain Shader positions vertices created by the Tessellator. It receives barycentric coordinates via `SV_DomainLocation`.

```hlsl
#pragma domain domain_shader

struct v2f
{
    UNITY_POSITION(pos);
    float2 uv_MainTex : TEXCOORD0;
    float4 tSpace0 : TEXCOORD1;
    float4 tSpace1 : TEXCOORD2;
    float4 tSpace2 : TEXCOORD3;
};

sampler2D _DispTex;
float _Displacement;

// Displacement function
void disp(inout appdata v)
{
    float d = tex2Dlod(_DispTex, float4(v.texcoord.xy, 0, 0)).r * _Displacement;
    v.vertex.xyz -= v.normal * d;
}

// Vertex processing (prepares data for fragment shader)
v2f vert_process(appdata v)
{
    v2f o;
    UNITY_INITIALIZE_OUTPUT(v2f, o);
    o.pos = UnityObjectToClipPos(v.vertex);
    o.uv_MainTex.xy = TRANSFORM_TEX(v.texcoord, _MainTex);

    float3 worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
    float3 worldNormal = UnityObjectToWorldNormal(v.normal);
    fixed3 worldTangent = UnityObjectToWorldDir(v.tangent.xyz);
    fixed tangentSign = v.tangent.w * unity_WorldTransformParams.w;
    fixed3 worldBinormal = cross(worldNormal, worldTangent) * tangentSign;

    o.tSpace0 = float4(worldTangent.x, worldBinormal.x, worldNormal.x, worldPos.x);
    o.tSpace1 = float4(worldTangent.y, worldBinormal.y, worldNormal.y, worldPos.y);
    o.tSpace2 = float4(worldTangent.z, worldBinormal.z, worldNormal.z, worldPos.z);

    return o;
}

// Domain Shader - positions tessellated vertices
[UNITY_domain("tri")]
v2f domain_shader(
    TessellationFactors tessFactors,
    const OutputPatch<InternalTessInterp_appdata, 3> vi,
    float3 bary : SV_DomainLocation
)
{
    appdata v;
    UNITY_INITIALIZE_OUTPUT(appdata, v);

    // Interpolate all attributes using barycentric coordinates
    v.vertex = vi[0].vertex * bary.x +
               vi[1].vertex * bary.y +
               vi[2].vertex * bary.z;

    v.tangent = vi[0].tangent * bary.x +
                vi[1].tangent * bary.y +
                vi[2].tangent * bary.z;

    v.normal = vi[0].normal * bary.x +
               vi[1].normal * bary.y +
               vi[2].normal * bary.z;

    v.texcoord = vi[0].texcoord * bary.x +
                 vi[1].texcoord * bary.y +
                 vi[2].texcoord * bary.z;

    // Apply displacement here
    disp(v);

    // Prepare for fragment shader
    v2f o = vert_process(v);
    return o;
}
```

### Understanding SV_DomainLocation

The `SV_DomainLocation` semantic provides barycentric coordinates for triangle patches:
- `bary.x` = weight for vertex 0
- `bary.y` = weight for vertex 1
- `bary.z` = weight for vertex 2
- Always: `bary.x + bary.y + bary.z = 1.0`

These coordinates are used to interpolate all vertex attributes (position, normal, UV, etc.) to the new tessellated vertex positions.

---

## Application Example: Fluid Displacement

The chapter includes an example combining tessellation with fluid simulation from Volume 1:

1. GPU fluid simulation outputs a height map
2. Unity's built-in Plane mesh receives tessellation
3. Displacement shader samples height map and offsets vertices

The result: A simple plane mesh seamlessly follows complex fluid motion without mesh distortion, thanks to GPU-side subdivision.

---

## Performance Considerations

| Factor | Impact |
|--------|--------|
| Tessellation level | Higher = more triangles, more compute |
| Displacement texture sampling | Use `tex2Dlod` to avoid mipmap issues |
| Frustum culling | `UnityEdgeLengthBasedTessCull` skips off-screen patches |
| Edge-based subdivision | Prevents over-tessellation of small screen areas |

---

## Key Takeaways

1. **Tessellation** subdivides geometry on GPU, reducing CPU-GPU bandwidth
2. **Hull Shader** defines how and how much to subdivide (patch constants)
3. **Tessellator** is fixed-function - performs actual subdivision
4. **Domain Shader** positions new vertices using barycentric interpolation
5. **Surface Shader wrappers** provide easy integration for common cases
6. **Displacement** is applied in Domain Shader after tessellation
7. **Edge-length-based tessellation** adapts detail to screen-space size
8. **Frustum culling** optimizes by skipping invisible patches

## Best Practices

- Use edge-length-based tessellation to maintain consistent screen-space detail
- Enable culling to skip off-screen geometry
- Balance tessellation level with performance requirements
- Combine with normal maps for detail that doesn't need geometric subdivision
- Use LOD systems for distance-based quality scaling

## References

- Unity Manual: Surface Shader Tessellation
  - https://docs.unity3d.com/Manual/SL-SurfaceShaderTessellation.html
- Microsoft DirectX 11 Tessellation
  - https://docs.microsoft.com/en-us/windows/desktop/direct3d11/direct3d-11-advanced-stages-tessellation
