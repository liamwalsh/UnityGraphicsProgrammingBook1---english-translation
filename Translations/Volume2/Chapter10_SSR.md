# Chapter 10: Image Effect Application - Screen Space Reflection (SSR)

**Author**: Komietty

**Sample Project**: "SSR" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter covers **Screen Space Reflection (SSR)** as an advanced application of Image Effects. Reflections and specular highlights are crucial for creating realistic 3D environments, yet physically accurate ray tracing requires enormous computational resources.

While Unity now supports tools like Octane Renderer for offline rendering, real-time applications still require clever approximations. SSR is a post-processing technique that achieves convincing reflections within the constraints of screen-space data.

### Chapter Structure

1. **Gaussian Blur** - Warm-up post-effect technique
2. **SSR Core Algorithm** - Ray marching in screen space
3. **Optimization Techniques** - Mipmap acceleration, binary search refinement

---

## Part 1: Gaussian Blur

### The Concept

Blur effects work by sampling neighboring pixels and blending their colors. The **kernel** determines the blending weights. Gaussian blur uses a normal distribution for natural-looking results.

### Gaussian Distribution

$$G(x) = \frac{1}{\sqrt{2\pi\sigma^2}} \exp\left(-\frac{x^2}{2\sigma^2}\right)$$

In practice, we approximate this with binomial distribution weights:

```hlsl
float4 x_blur (v2f i) : SV_Target
{
    float weight[5] = { 0.2270270, 0.1945945, 0.1216216, 0.0540540, 0.0162162 };
    float offset[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    float2 size = _MainTex_TexelSize;

    fixed4 col = tex2D(_MainTex, i.uv) * weight[0];
    for(int j = 1; j < 5; j++)
    {
        col += tex2D(_MainTex, i.uv + float2(offset[j], 0) * size) * weight[j];
        col += tex2D(_MainTex, i.uv - float2(offset[j], 0) * size) * weight[j];
    }
    return col;
}
```

### Separable Blur Optimization

By splitting into X and Y passes, we reduce texture samples from n x n to 2n + 1:

```csharp
void OnRenderImage(RenderTexture src, RenderTexture dst)
{
    var rt = RenderTexture.GetTemporary(src.width, src.height, 0, src.format);

    for (int i = 0; i < blurNum; i++)
    {
        Graphics.Blit(src, rt, mat, 0);  // X blur
        Graphics.Blit(rt, src, mat, 1);  // Y blur
    }
    Graphics.Blit(src, dst);

    RenderTexture.ReleaseTemporary(rt);
}
```

---

## Part 2: SSR Fundamentals

### Required Buffers

SSR needs:
- **Color buffer** - The rendered image
- **Depth buffer** - Per-pixel depth values
- **Normal buffer** - Per-pixel surface normals

These G-buffers are essential for deferred rendering techniques.

### The Core Idea

Unlike physical optics where light travels from source to eye, SSR works **backwards**:
1. For each pixel, determine the reflection direction
2. March a ray along that direction
3. Find where the ray hits geometry (using depth comparison)
4. Sample the color at that hit point

### Algorithm Overview

1. Convert screen coordinates to world space using depth
2. Calculate reflection vector from view direction and normal
3. Step the ray forward in small increments
4. Transform ray position back to screen space
5. Compare ray depth with depth buffer
6. If ray is behind geometry, we found a reflection source
7. Sample and apply that color to the original pixel

---

## Part 3: Implementation

### Coordinate Transformation Setup

```csharp
void OnRenderImage(RenderTexture src, RenderTexture dst)
{
    var view = cam.worldToCameraMatrix;
    var proj = GL.GetGPUProjectionMatrix(cam.projectionMatrix, false);
    var viewProj = proj * view;

    mat.SetMatrix("_ViewProj", viewProj);
    mat.SetMatrix("_InvViewProj", viewProj.inverse);
    // ...
}
```

### Computing Reflection Direction

```hlsl
float4 reflection (v2f i) : SV_Target
{
    float2 uv = i.screen.xy / i.screen.w;
    float depth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv);

    // Reconstruct world position
    float2 screenpos = 2.0 * uv - 1.0;
    float4 pos = mul(_InvViewProj, float4(screenpos, depth, 1.0));
    pos /= pos.w;

    // Calculate view and reflection directions
    float3 camDir = normalize(pos - _WorldSpaceCameraPos);
    float3 normal = tex2D(_CameraGBufferTexture2, uv) * 2.0 - 1.0;
    float3 refDir = reflect(camDir, normal);

    // Debug visualization
    if (_ViewMode == 1) return float4((normal.xyz * 0.5 + 0.5), 1);
    if (_ViewMode == 2) return float4((refDir.xyz * 0.5 + 0.5), 1);
    // ...
}
```

### Ray Marching Loop

```hlsl
[loop]
for (int n = 1; n <= _MaxLoop; n++)
{
    float3 step = refDir * _RayLenCoeff * (lod + 1);
    ray += step * (1 + rand(uv + _Time.x) * (1 - smooth));

    // Transform ray to screen space
    float4 rayScreen = mul(_ViewProj, float4(ray, 1.0));
    float2 rayUV = rayScreen.xy / rayScreen.w * 0.5 + 0.5;
    float rayDepth = ComputeDepth(rayScreen);

    // Sample world depth at ray position
    float worldDepth = (lod == 0) ?
        SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, rayUV) :
        tex2Dlod(_CameraDepthMipmap, float4(rayUV, 0, lod)) + _BaseRaise * lod;

    // Check for intersection
    if(rayDepth < worldDepth)
    {
        // Ray has passed behind geometry - found reflection!
        // ...
        return outcol;
    }
}
```

**Note**: The `[loop]` attribute is required when loop count is determined at runtime.

---

## Part 4: Optimization Techniques

### Challenge 1: Limited Ray Distance

With fixed loop count, rays can't travel far enough for distant reflections.

### Challenge 2: Stepping Through Objects

Large steps may skip thin objects, sampling incorrect colors.

### Challenge 3: Performance

Many iterations per pixel is expensive.

### Solution: Hierarchical Ray Marching with Mipmaps

Use **mipmaps** (lower-resolution versions of the depth buffer) to take larger steps in empty space, then refine with smaller steps near geometry.

#### Creating Mipmapped RenderTexture

```csharp
void OnEnable()
{
    rt = new RenderTexture(Screen.width, Screen.height, 24);
    rt.useMipMap = true;  // Enable mipmaps
}
```

#### LOD-Based Stepping

```hlsl
for (int n = 1; n <= _MaxLoop; n++)
{
    float3 step = refDir * _RayLenCoeff * (lod + 1);  // Larger steps at higher LOD
    ray += step;

    if(rayDepth < worldDepth)  // Intersection detected
    {
        if(lod == 0)
        {
            // Final refinement at full resolution
            // Binary search for precise hit point
            break;
        }
        else
        {
            ray -= step;  // Back up
            lod--;        // Decrease LOD for finer stepping
        }
    }
    else if(n <= _MaxLOD)
    {
        lod++;  // Increase LOD for larger steps in empty space
    }
}
```

### Binary Search Refinement

Once we detect intersection at LOD 0, use binary search to find the precise hit point:

```hlsl
if (lod == 0)
{
    if (rayDepth + _Thickness > worldDepth)
    {
        float sign = -1.0;
        for (int m = 1; m <= 8; ++m)
        {
            ray += sign * pow(0.5, m) * step;

            rayScreen = mul(_ViewProj, float4(ray, 1.0));
            rayUV = rayScreen.xy / rayScreen.w * 0.5 + 0.5;
            rayDepth = ComputeDepth(rayScreen);
            worldDepth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, rayUV);

            sign = (rayDepth < worldDepth) ? -1 : 1;
        }
        refcol = tex2D(_MainTex, rayUV);
    }
    break;
}
```

---

## Part 5: Material-Based Reflection

### Using Smoothness from G-Buffer

Different materials should reflect differently. The G-buffer stores material properties:

```hlsl
// Sample material smoothness
float smooth = tex2D(_CameraGBufferTexture1, uv).w;

// Blend reflection based on smoothness
return (col * (1 - smooth) + refcol * smooth) * _ReflectionRate
     + col * (1 - _ReflectionRate);
```

---

## Part 6: Edge-Aware Blur

Apply blur to hide artifacts from step size limitations, but preserve edges:

```hlsl
float4 xblur(v2f i) : SV_Target
{
    float2 uv = i.screen.xy / i.screen.w;
    float smooth = tex2D(_CameraGBufferTexture1, uv).w;

    // Edge detection via depth difference
    float depth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv);
    float depthR = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv + float2(1, 0) * size);
    float depthL = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv - float2(1, 0) * size);

    float4 originalColor = tex2D(_ReflectionTexture, uv);
    float4 blurredColor = /* Gaussian blur calculation */;

    // Skip blur at edges (large depth discontinuity)
    float4 o = (abs(depthR - depthL) > _BlurThreshold) ? originalColor
             : blurredColor * smooth + originalColor * (1 - smooth);
    return o;
}
```

---

## Bonus: Two-Camera Reflections

An experimental technique using a secondary camera to reflect objects not visible to the main camera:

```hlsl
// Use sub-camera's depth and color buffers
float subCameraDepth = SAMPLE_DEPTH_TEXTURE(_SubCameraDepthTex, rayUv);

if (rayDepth < subCameraDepth && rayDepth + thickness > subCameraDepth)
{
    // Binary search refinement...
    col = tex2D(_SubCameraMainTex, rayUv);
}
```

**Limitation**: Objects that should be occluded may still appear in reflections.

---

## Key Takeaways

| Technique | Purpose |
|-----------|---------|
| Ray Marching | Core SSR algorithm - step along reflection vector |
| Depth Comparison | Detect where rays intersect geometry |
| Hierarchical Tracing | Mipmap-based acceleration for longer rays |
| Binary Search | Precise intersection point refinement |
| Material Smoothness | Vary reflection intensity per-material |
| Edge-Aware Blur | Hide artifacts while preserving detail |

---

## Performance Considerations

SSR is computationally expensive because:
- Processing scales with screen resolution
- Each pixel may require many ray steps
- Multiple texture samples per step

**Optimization strategies**:
- Limit maximum ray steps
- Use hierarchical depth (mipmaps)
- Reduce reflection resolution
- Blur to hide low sample counts

---

## Summary

SSR achieves real-time reflections within screen-space constraints. While it cannot reflect objects outside the view frustum, it provides convincing results for floors, water, and glossy surfaces.

The techniques covered - ray marching, hierarchical acceleration, binary search refinement - extend beyond SSR to other screen-space effects like ambient occlusion and global illumination approximations.

Experiment with the sample project parameters to understand the trade-offs between quality and performance.

---

## References

- Kode80: "Screen Space Reflections in Unity 5"
- Chalmers University: "Screen-space reflections" course materials
- hecomi: "Unity Screen Space Reflection Implementation"
- Unity Manual: Deferred Shading G-Buffer layout
