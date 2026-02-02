# Chapter 9: Image Effect Introduction

**Author**: (SimpleImageEffect)

**Sample Project**: "SimpleImageEffect" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter provides a simple explanation of how to implement **Image Effects** (also called **Post Effects**) in Unity - the technique of applying GPU shaders to the rendered output.

Image Effects are used for:
- Glow/bloom effects
- Anti-aliasing
- Depth of Field (DOF)
- Color correction and grading
- Many other visual enhancements

The simplest examples involve color modifications, which this chapter covers.

**Prerequisites**: Basic familiarity with Unlit or Surface shaders is helpful, but the simple structure makes this accessible even without prior shader experience.

---

## How Image Effects Work

Image Effects process the rendered image pixel-by-pixel. Essentially, implementing an Image Effect means implementing a **fragment shader** that operates on the screen output.

---

## Unity's Image Effect Pipeline

1. Camera renders the scene
2. Rendered content is passed to `OnRenderImage` method
3. Image Effect shader processes the input
4. `OnRenderImage` outputs the modified image

---

## Basic Implementation

### The C# Script

**Scene**: "ImageEffectBase"

```csharp
[ExecuteInEditMode]
[RequireComponent(typeof(Camera))]
public class ImageEffectBase : MonoBehaviour
{
    public Material material;

    protected virtual void Start()
    {
        if (!SystemInfo.supportsImageEffects
         || !this.material
         || !this.material.shader.isSupported)
        {
            base.enabled = false;
        }
    }

    protected virtual void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        Graphics.Blit(source, destination, this.material);
    }
}
```

### Key Components

**Attributes**:
- `[ExecuteInEditMode]` - See effect in Scene view without playing
- `[RequireComponent(typeof(Camera))]` - `OnRenderImage` only works with cameras

**OnRenderImage Parameters**:
- `source` - The camera's rendered output
- `destination` - Where to write the result (null = screen buffer)

**Graphics.Blit**:
- Renders `source` to `destination` using the specified material
- Must always write something to `destination`

### Validation Note

The `Start()` validation disables the effect on unsupported hardware. Be aware that `SystemInfo.supportsImageEffects` checks might fail in `Awake` or `OnEnable` due to `ExecuteInEditMode` timing.

---

## The Simplest Image Effect Shader

```hlsl
Shader "ImageEffectBase"
{
    Properties
    {
        _MainTex("Texture", 2D) = "white" {}
    }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always

        Pass
        {
            CGPROGRAM

            #include "UnityCG.cginc"
            #pragma vertex vert_img
            #pragma fragment frag

            sampler2D _MainTex;

            fixed4 frag(v2f_img input) : SV_Target
            {
                float4 color = tex2D(_MainTex, input.uv);
                color.rgb = 1 - color.rgb;  // Invert colors
                return color;
            }

            ENDCG
        }
    }
}
```

### Understanding the Shader

**`_MainTex`**: Reserved name for `Graphics.Blit` input. Changing this name breaks the pipeline.

**`Cull Off ZWrite Off ZTest Always`**: Unity recommends these settings to prevent unintended depth buffer interactions.

**`#pragma vertex vert_img`**: Uses Unity's built-in vertex shader from UnityCG.cginc - handles the full-screen quad rendering.

**`v2f_img`**: Built-in structure containing position and UV coordinates.

### Why the Simplified Version Works

Unity's default Image Effect shader includes a complete vertex shader:

```hlsl
struct appdata
{
    float4 vertex : POSITION;
    float2 uv : TEXCOORD0;
};

struct v2f
{
    float2 uv : TEXCOORD0;
    float4 vertex : SV_POSITION;
};

v2f vert (appdata v)
{
    v2f o;
    o.vertex = UnityObjectToClipPos(v.vertex);
    o.uv = v.uv;
    return o;
}
```

The vertex shader just creates a screen-aligned quad with UV coordinates - rarely needs modification. UnityCG.cginc provides `vert_img` and `v2f_img` for this common case.

---

## Practice: Diagonal Split Effect

Let's modify the effect to only invert the diagonal half of the screen.

### Understanding UV Coordinates

`input.uv` contains normalized (0-1) coordinates where:
- Origin (0,0) = bottom-left
- (1,1) = top-right

### Horizontal/Vertical Split

```hlsl
// Invert left half
color.rgb = input.uv.x < 0.5 ? 1 - color.rgb : color.rgb;

// Invert bottom half
color.rgb = input.uv.y < 0.5 ? 1 - color.rgb : color.rgb;
```

### Diagonal Split

```hlsl
color.rgb = input.uv.y < input.uv.x ? 1 - color.rgb : color.rgb;
```

---

## Useful Built-in Values

### _ScreenParams

`float4` containing:
- `x` = pixel width
- `y` = pixel height
- `z` = 1 + 1/width
- `w` = 1 + 1/height

Useful for aspect ratio calculations and pixel-based effects.

### _MainTex_TexelSize

`float4` containing:
- `x` = 1/width
- `y` = 1/height
- `z` = width
- `w` = height

**Automatically defined** for any `sampler2D` - just add `_TexelSize` suffix.

### Sampling Adjacent Pixels

```hlsl
sampler2D _MainTex;
float4    _MainTex_TexelSize;

fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);

    // Sample 4 neighbors
    color += tex2D(_MainTex, input.uv + float2(_MainTex_TexelSize.x, 0));
    color += tex2D(_MainTex, input.uv - float2(_MainTex_TexelSize.x, 0));
    color += tex2D(_MainTex, input.uv + float2(0, _MainTex_TexelSize.y));
    color += tex2D(_MainTex, input.uv - float2(0, _MainTex_TexelSize.y));

    return color / 5;  // Average (smoothing filter)
}
```

This pattern is used for blur filters, edge detection, and many other effects.

---

## Accessing Depth and Normals

### G-Buffer Concept

The G-Buffer stores per-pixel information beyond color:
- Depth buffer
- Normal buffer
- Other material properties

This data enables effects like ambient occlusion, screen-space reflections, and more.

### Enabling Depth/Normal Access

```csharp
public class ImageEffect : ImageEffectBase
{
    protected new Camera camera;
    public DepthTextureMode depthTextureMode;

    protected override void Start()
    {
        base.Start();
        this.camera = base.GetComponent<Camera>();
        this.camera.depthTextureMode = this.depthTextureMode;
    }
}
```

**DepthTextureMode Options**:
- `None` (default)
- `Depth` - depth only
- `DepthNormals` - depth and normals combined
- `MotionVectors` - per-pixel motion

### Reading Depth and Normals in Shader

```hlsl
sampler2D _MainTex;
sampler2D _CameraDepthNormalsTexture;

fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);
    float3 normal;
    float  depth;

    // Decode combined depth+normal texture
    DecodeDepthNormal(
        tex2D(_CameraDepthNormalsTexture, input.uv),
        depth,
        normal
    );

    // Normalize depth to 0-1 range (platform-independent)
    depth = Linear01Depth(depth);

    // Visualize depth
    return fixed4(depth, depth, depth, 1);

    // Or visualize normals
    return fixed4(normal.xyz, 1);
}
```

### Depth Visualization

- Near objects: depth closer to 1 (white)
- Far objects: depth closer to 0 (black)
- Affected by camera's near/far clip planes

### Normal Visualization

Normal components (X, Y, Z) map to (R, G, B):
- Surfaces facing right: more red
- Surfaces facing up: more green
- Surfaces facing camera: more blue

---

## Key Takeaways

| Concept | Description |
|---------|-------------|
| OnRenderImage | Unity callback for post-processing |
| Graphics.Blit | Renders texture through material to destination |
| _MainTex | Reserved name for Blit input |
| vert_img | Built-in vertex shader for full-screen effects |
| _TexelSize | Automatic pixel size for any sampler2D |
| DepthTextureMode | Enables G-buffer access |
| DecodeDepthNormal | Extracts depth and normal from combined texture |

---

## Summary

Image Effects transform rendered output through fragment shaders. The pipeline is straightforward:

1. Camera renders to texture
2. `OnRenderImage` receives the texture
3. `Graphics.Blit` processes it through your shader
4. Result goes to screen (or next effect)

With depth and normal access, you can create sophisticated effects like ambient occlusion, fog, outlines, and more - topics explored in the next chapter.

---

## References

- [Writing Image Effects - Unity Documentation](https://docs.unity3d.com/Manual/WritingImageEffects.html)
- [Accessing shader properties in Cg/HLSL](https://docs.unity3d.com/Manual/SL-PropertiesInPrograms.html)
- [Using Depth Textures](https://docs.unity3d.com/Manual/SL-DepthTextures.html)
