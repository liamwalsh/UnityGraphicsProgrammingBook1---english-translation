# Chapter 3: Screen Space Fluid Rendering

**Author: Oishi**

## Introduction

This chapter introduces **Screen Space Fluid Rendering** using **Deferred Shading** as a technique for rendering particles as fluid-like surfaces.

## What is Screen Space Fluid Rendering?

Traditional methods for rendering continuous bodies like fluids use techniques like Marching Cubes, but these are computationally expensive and not ideal for real-time applications requiring fine detail.

**Screen Space Fluid Rendering** was developed as a faster alternative for particle-based fluids. The core idea: generate surfaces from the depth of particles as seen from the camera in screen space.

## Deferred Rendering Overview

### Concept

Deferred Rendering performs **shading calculations in 2D screen space** rather than during geometry processing. Traditional rendering is called **Forward Rendering** for distinction.

### Pipeline Comparison

**Forward Rendering**:
- First pass: Lighting and shading during geometry processing

**Deferred Rendering**:
- First pass (G-Buffer pass): Generate 2D images of normals, positions, depth, diffuse color
- Second pass: Perform lighting/shading using G-Buffer data
- The "deferred" name comes from actual rendering being delayed to later passes

### Advantages

- Supports many light sources efficiently
- Only calculates shading for visible pixels (minimizes fragment shader executions)
- Enables geometry modification
- G-Buffer data available for post-effects (SSAO, etc.)

### Disadvantages

- Weak at semi-transparent rendering
- Some anti-aliasing algorithms (MSAA) don't work well
- Difficult to use multiple materials
- No support for orthographic cameras

### Unity Requirements

- Unity Pro only
- Multiple Render Targets (MRT) support
- Shader Model 3.0+
- Graphics card supporting depth render textures and two-sided stencil buffers

## G-Buffer (Geometry Buffer)

The G-Buffer stores screen-space 2D texture information for shading: normals, positions, diffuse reflectance, etc.

### Default Unity G-Buffer Structure

| Render Target | Format | Data Type |
|---------------|--------|-----------|
| RT0 | ARGB32 | Diffuse color (RGB), Occlusion (A) |
| RT1 | ARGB32 | Specular color (RGB), Roughness (A) |
| RT2 | ARGB2101010 | World space normal (RGB) |
| RT3 | ARGB2101010 | Emission + lighting |
| Z-buffer | - | Depth + Stencil |

### Accessing G-Buffer in Shaders

| Shader Property Name | Data Type |
|---------------------|-----------|
| _CameraGBufferTexture0 | Diffuse color (RGB), occlusion (A) |
| _CameraGBufferTexture1 | Specular color (RGB) |
| _CameraGBufferTexture2 | World space normal (RGB) |
| _CameraGBufferTexture3 | Emission + lighting |
| _CameraDepthTexture | Depth + Stencil |

## CommandBuffer

CPU scripts don't perform actual drawing - they add rendering commands to a **graphics command buffer** that the GPU reads and executes.

Unity's `CommandBuffer` API lets you insert command lists at specific points in the rendering pipeline, extending Unity's built-in pipeline.

Key methods include `Graphics.DrawMesh()` and `Graphics.DrawProcedural()`.

## Coordinate Systems and Transformations

Understanding coordinate transformations is essential for screen-space calculations.

### Homogeneous Coordinates

3D position vectors (x,y,z) are represented as 4D (x,y,z,w) for efficient matrix multiplication. Transform: (x/w, y/w, z/w, 1) = (x, y, z, w)

### Coordinate Spaces

1. **Object Space** (Local/Model): Object-centered coordinates
2. **World Space** (Global): Scene-centered coordinates
3. **Eye/View Space** (Camera): Camera-centered coordinates
4. **Clip Space**: After projection, used for clipping
5. **Normalized Device Coordinates (NDC)**: -1 to 1 range after perspective divide
6. **Screen/Window Space**: Final pixel coordinates

### Transformation Flow

Object Space → (Model Transform) → World Space → (View Transform) → Eye Space → (Projection Transform) → Clip Space → (Perspective Divide) → NDC → (Viewport Transform) → Screen Space

## Implementation

### Algorithm Overview

1. Render particles to generate screen-space depth image
2. Apply blur to smooth the depth
3. Calculate surface normals from depth
4. Calculate surface shading

### Script Structure

| Script | Function |
|--------|----------|
| ScreenSpaceFluidRenderer.cs | Main controller |
| RenderParticleDepth.shader | Calculate screen-space particle depth |
| BilateralFilterBlur.shader | Depth-attenuating blur effect |
| CalcNormal.shader | Calculate normals from depth |
| RenderGBuffer.shader | Write depth, normals, color to G-Buffer |

### Setting Up CommandBuffer

```csharp
void OnWillRenderObject()
{
    var act = gameObject.activeInHierarchy && enabled;
    if (!act)
    {
        CleanUp();
        return;
    }

    var cam = Camera.current;
    if (!cam) return;

    if (!_cameras.ContainsKey(cam))
    {
        var buf = new CmdBufferInfo();
        buf.pass = CameraEvent.BeforeGBuffer;
        buf.buffer = new CommandBuffer();
        buf.name = "Screen Space Fluid Renderer";

        // Insert before G-Buffer generation
        cam.AddCommandBuffer(buf.pass, buf.buffer);
        _cameras.Add(cam, buf);
    }
}
```

The key is `CameraEvent.BeforeGBuffer` - inserting our custom rendering right before Unity generates the standard G-Buffer.

### Step 1: Generate Particle Depth Image

```csharp
// Get shader property ID
int depthBufferId = Shader.PropertyToID("_DepthBuffer");

// Create temporary RenderTexture
buf.GetTemporaryRT(depthBufferId, -1, -1, 24,
    FilterMode.Point, RenderTextureFormat.RFloat);

// Set render targets
buf.SetRenderTarget(
    new RenderTargetIdentifier(depthBufferId),
    new RenderTargetIdentifier(depthBufferId)
);
buf.ClearRenderTarget(true, true, Color.clear);

// Set particle data
_renderParticleDepthMaterial.SetFloat("_ParticleSize", _particleSize);
_renderParticleDepthMaterial.SetBuffer("_ParticleDataBuffer",
    _particleControllerScript.GetParticleDataBuffer());

// Draw particles as point sprites
buf.DrawProcedural(
    Matrix4x4.identity,
    _renderParticleDepthMaterial,
    0,
    MeshTopology.Points,
    _particleControllerScript.GetMaxParticleNum()
);
```

### Point Sprite Generation (Geometry Shader)

```hlsl
static const float3 g_positions[4] =
{
    float3(-1, 1, 0),
    float3( 1, 1, 0),
    float3(-1,-1, 0),
    float3( 1,-1, 0),
};

[maxvertexcount(4)]
void geom(point v2g In[1], inout TriangleStream<g2f> SpriteStream)
{
    g2f o = (g2f)0;
    float3 vertpos = In[0].position.xyz;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float3 pos = g_positions[i] * _ParticleSize;
        pos = mul(unity_CameraToWorld, pos) + vertpos;
        o.position = UnityObjectToClipPos(float4(pos, 1.0));
        o.uv = g_texcoords[i];
        o.vpos = UnityObjectToViewPos(float4(pos, 1.0)).xyz;
        o.size = _ParticleSize;

        SpriteStream.Append(o);
    }
    SpriteStream.RestartStrip();
}
```

### Depth Calculation (Fragment Shader)

```hlsl
fragmentOut frag(g2f i)
{
    // Calculate hemisphere normal from UV
    float3 N = (float3)0;
    N.xy = i.uv.xy * 2.0 - 1.0;
    float radius_sq = dot(N.xy, N.xy);
    if (radius_sq > 1.0) discard;
    N.z = sqrt(1.0 - radius_sq);

    // Pixel position in clip space
    float4 pixelPos = float4(i.vpos.xyz + N * i.size, 1.0);
    float4 clipSpacePos = mul(UNITY_MATRIX_P, pixelPos);
    float depth = clipSpacePos.z / clipSpacePos.w;

    fragmentOut o = (fragmentOut)0;
    o.depthBuffer = depth;
    o.depthStencil = depth;

    return o;
}
```

### Step 2: Blur the Depth Image

A bilateral filter blur smooths the depth while preserving edges, connecting adjacent particles into a continuous surface.

### Step 3: Calculate Normals from Depth

Normals are calculated using **partial derivatives** in X and Y directions.

```hlsl
float3 uvToEye(float2 uv, float z)
{
    float2 xyPos = uv * 2.0 - 1.0;
    float4 clipPos = float4(xyPos.xy, z, 1.0);
    float4 viewPos = mul(unity_CameraInvProjection, clipPos);
    viewPos.xyz = viewPos.xyz / viewPos.w;
    return viewPos.xyz;
}

float3 getEyePos(float2 uv)
{
    return uvToEye(uv, sampleDepth(uv));
}

float4 frag(v2f_img i) : SV_Target
{
    float2 uv = i.uv.xy;
    float depth = tex2D(_DepthBuffer, uv);

    #if UNITY_REVERSED_Z
    if (Linear01Depth(depth) > 1.0 - 1e-3) discard;
    #else
    if (Linear01Depth(depth) < 1e-3) discard;
    #endif

    float2 ts = _DepthBuffer_TexelSize.xy;
    float3 posEye = getEyePos(uv);

    // Partial derivative in X
    float3 ddx = getEyePos(uv + float2(ts.x, 0.0)) - posEye;
    float3 ddx2 = posEye - getEyePos(uv - float2(ts.x, 0.0));
    ddx = abs(ddx.z) > abs(ddx2.z) ? ddx2 : ddx;

    // Partial derivative in Y
    float3 ddy = getEyePos(uv + float2(0.0, ts.y)) - posEye;
    float3 ddy2 = posEye - getEyePos(uv - float2(0.0, ts.y));
    ddy = abs(ddy.z) > abs(ddy2.z) ? ddy2 : ddy;

    // Cross product gives normal
    float3 N = normalize(cross(ddx, ddy));
    N = normalize(mul(_ViewMatrix, float4(N, 0.0)));

    return float4(N * 0.5 + 0.5, 1.0);
}
```

### Step 4: Write to G-Buffer

```csharp
buf.SetGlobalTexture("_NormalBuffer", normalBufferId);
buf.SetGlobalTexture("_DepthBuffer", depthBufferId);

_renderGBufferMaterial.SetColor("_Diffuse", _diffuse);
_renderGBufferMaterial.SetColor("_Specular",
    new Vector4(_specular.r, _specular.g, _specular.b, 1.0f - _roughness));
_renderGBufferMaterial.SetColor("_Emission", _emission);

// Set G-Buffer as render targets
buf.SetRenderTarget(
    new RenderTargetIdentifier[4]
    {
        BuiltinRenderTextureType.GBuffer0, // Diffuse
        BuiltinRenderTextureType.GBuffer1, // Specular + Roughness
        BuiltinRenderTextureType.GBuffer2, // World Normal
        BuiltinRenderTextureType.GBuffer3  // Emission
    },
    BuiltinRenderTextureType.CameraTarget  // Depth
);

buf.DrawMesh(quad, Matrix4x4.identity, _renderGBufferMaterial);
```

### G-Buffer Output Structure

```hlsl
struct gbufferOut
{
    half4 diffuse  : SV_Target0;
    half4 specular : SV_Target1;
    half4 normal   : SV_Target2;
    half4 emission : SV_Target3;
    float depth    : SV_Depth;
};
```

### Memory Cleanup

Always release temporary RenderTextures with `ReleaseTemporaryRT()` to prevent memory overflow.

## Key Takeaways

1. **Screen Space Fluid Rendering** creates fluid surfaces from particle depth in screen space
2. **Deferred Rendering** enables screen-space geometry manipulation
3. **G-Buffer** stores geometry information for deferred shading calculations
4. **CommandBuffer** extends Unity's rendering pipeline at specific points
5. **Normal calculation from depth** uses partial derivatives (ddx/ddy)
6. Understanding **coordinate transformations** is essential for screen-space techniques

## Limitations and Future Work

This sample focuses on deferred geometry manipulation. For realistic liquid rendering, additional features would include:
- Light absorption based on thickness
- Internal refraction
- Background transparency
- Caustics effects

## References

- GDC: Screen Space Fluid Rendering for Games (Simon Green, NVIDIA)
- Real-time Rendering Fundamentals (Satoshi Kodaira)
