# Chapter 4: StarGlow

**Author:** Xjine
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** StarGlow

---

## Introduction

When strong light reflects, it often produces rays extending outward. This effect goes by many names - Light Leak, Light Streak, or StarGlow. This chapter demonstrates how to create this effect as a post-process. We'll call it **StarGlow**.

This post-effect was presented by Masaki Kawase at GDC 2003 and remains an elegant technique for achieving dramatic lighting effects.

---

## Overview

The StarGlow effect works in four main steps:

1. **Extract brightness** - Create a texture containing only bright pixels
2. **Directional blur** - Apply streaking blur in specific directions
3. **Composite streaks** - Combine multiple blur directions
4. **Final composite** - Blend result with original image

---

## STEP 1: Generate Brightness Image

First, detect and extract only the bright parts of the image. This is similar to what's done for general glow/bloom effects.

### C# Setup

```csharp
// Create reduced-resolution RenderTexture for performance
RenderTexture brightnessTex = RenderTexture.GetTemporary(
    source.width / this.divide,
    source.height / this.divide,
    source.depth,
    source.format
);

// Pass parameters to shader
// x = threshold, y = intensity, z = attenuation
material.SetVector(this.idParameter,
    new Vector3(threshold, intensity, attenuation));

Graphics.Blit(source, brightnessTex, material, 1);  // Pass 1
```

### Shader (Pass 1)

```hlsl
#define BRIGHTNESS_THRESHOLD _Parameter.x
#define INTENSITY            _Parameter.y
#define ATTENUATION          _Parameter.z

fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);
    return max(color - BRIGHTNESS_THRESHOLD, 0) * INTENSITY;
}
```

### How It Works

- **BRIGHTNESS_THRESHOLD** - Cutoff value; pixels below this become black
- **INTENSITY** - Multiplier for surviving brightness values
- **ATTENUATION** - Used later for blur decay

Brighter pixels produce larger values. Higher thresholds reduce the number of visible bright spots. Higher intensity makes the glow stronger.

### Critical Optimization

**The brightness texture is created at reduced resolution.** Post-effects scale with pixel count - higher resolution means more fragment shader calls. Since glow involves iterative processing, this cost multiplies. Reducing resolution significantly improves performance with minimal visual impact.

---

## STEP 2: Apply Directional Blur

Apply a blur that stretches in a specific direction, creating the characteristic star rays rather than the omnidirectional blur of typical glow.

### C# Setup

```csharp
Vector2 offset = new Vector2(-1, -1);  // Blur direction
// In practice: Quaternion.AngleAxis(angle, Vector3.forward) * Vector2.down

material.SetVector(this.idOffset, offset);
material.SetInt(this.idIteration, 1);

Graphics.Blit(brightnessTex, blurredTex1, material, 2);  // Pass 2

// Repeat for additional stretching
for (int i = 2; i <= this.iteration; i++)
{
    material.SetInt(this.idIteration, i);
    Graphics.Blit(blurredTex1, blurredTex2, material, 2);

    // Swap buffers
    RenderTexture temp = blurredTex1;
    blurredTex1 = blurredTex2;
    blurredTex2 = temp;
}
```

### Shader (Pass 2)

```hlsl
int    _Iteration;
float2 _Offset;

struct v2f_starglow
{
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
    half   power  : TEXCOORD1;
    half2  offset : TEXCOORD2;
};

v2f_starglow vert(appdata_img v)
{
    v2f_starglow o;
    o.pos = UnityObjectToClipPos(v.vertex);
    o.uv = v.texcoord;

    // Power increases exponentially with iteration
    o.power = pow(4, _Iteration - 1);

    // Offset scales with pixel size and power
    o.offset = _MainTex_TexelSize.xy * _Offset * o.power;

    return o;
}

float4 frag(v2f_starglow input) : SV_Target
{
    half4 color = half4(0, 0, 0, 0);
    half2 uv = input.uv;

    // Sample 4 pixels along the offset direction
    for (int j = 0; j < 4; j++)
    {
        color += saturate(tex2D(_MainTex, uv)
               * pow(ATTENUATION, input.power * j));
        uv += input.offset;
    }

    return color;
}
```

### Understanding the Blur

**Vertex Shader:**
- `power` = 4^(iteration-1) - Controls how far to sample
- `offset` = pixel size x direction x power - The step between samples

Calculating these in the vertex shader avoids redundant computation in the fragment shader.

**Fragment Shader:**
- Samples 4 pixels along the offset direction
- Each sample is attenuated: sample 0 gets ATTENUATION^0, sample 1 gets ATTENUATION^1, etc.
- Results are accumulated

**Example with ATTENUATION = 0.7:**
- Sample 0: x 0.7^0 = x 1.0
- Sample 1: x 0.7^1 = x 0.7
- Sample 2: x 0.7^2 = x 0.49
- Sample 3: x 0.7^3 = x 0.34

This creates a gradient that fades along the streak direction.

### Iterative Stretching

Each iteration:
1. Takes the already-blurred image as input
2. Increases `power`, sampling further away
3. Produces progressively longer streaks

The 4-sample count per pass is from Kawase's original presentation. Typically 2-3 total iterations produce good results while remaining performant.

---

## STEP 2.5: Composite Multiple Directions

For a proper star effect, we need streaks in multiple directions. We repeat the blur process with different `offset` directions and composite the results.

### C# Implementation

```csharp
for (int x = 1; x <= this.numOfStreak; x++)
{
    // Calculate offset direction for this streak
    Vector2 offset = Quaternion.AngleAxis(
        angle * x + this.angleOfStreak,
        Vector3.forward
    ) * Vector2.down;
    offset = offset.normalized;

    // Apply iterative blur for this direction
    // ...

    // Composite to accumulated texture
    Graphics.Blit(blurredTex1, compositeTex, material, 3);  // Pass 3
}
```

### Shader (Pass 3) - Soft Blending

```hlsl
Blend OneMinusDstColor One

fixed4 frag(v2f_img input) : SV_Target
{
    return tex2D(_MainTex, input.uv);
}
```

The blending mode `OneMinusDstColor One` creates a soft, screen-like blend. This can be customized based on desired artistic effect.

---

## STEP 3: Final Composite

Blend the accumulated streak texture with the original image.

### C# Setup

```csharp
// Enable selected composite mode
material.EnableKeyword(StarGlow.CompositeTypes[this.compositeType]);
material.SetColor(this.idCompositeColor, this.color);
material.SetTexture(this.idCompositeTex, compositeTex);

Graphics.Blit(source, destination, material, 4);  // Pass 4
```

### Shader (Pass 4) - Multiple Composite Modes

```hlsl
#pragma multi_compile _COMPOSITE_TYPE_ADDITIVE _COMPOSITE_TYPE_SCREEN ...

fixed4 frag(v2f_img input) : SV_Target
{
    float4 mainColor = tex2D(_MainTex, input.uv);
    float4 compositeColor = tex2D(_CompositeTex, input.uv);

    // Optional color tinting
    #if defined(_COMPOSITE_TYPE_COLORED_ADDITIVE) \
     || defined(_COMPOSITE_TYPE_COLORED_SCREEN)
    compositeColor.rgb =
        (compositeColor.r + compositeColor.g + compositeColor.b)
        * 0.3333 * _CompositeColor;
    #endif

    // Screen blend
    #if defined(_COMPOSITE_TYPE_SCREEN) \
     || defined(_COMPOSITE_TYPE_COLORED_SCREEN)
    return saturate(mainColor + compositeColor
                  - saturate(mainColor * compositeColor));

    // Additive blend
    #elif defined(_COMPOSITE_TYPE_ADDITIVE) \
       || defined(_COMPOSITE_TYPE_COLORED_ADDITIVE)
    return saturate(mainColor + compositeColor);

    // Debug: show only streaks
    #else
    return compositeColor;
    #endif
}
```

This provides flexibility for:
- **Additive** - Simple brightness addition
- **Screen** - Softer, more natural blend
- **Colored variants** - Apply tint color to streaks

---

## STEP 4: Resource Cleanup

Always release temporary resources:

```csharp
material.DisableKeyword(StarGlow.CompositeTypes[this.compositeType]);

RenderTexture.ReleaseTemporary(brightnessTex);
RenderTexture.ReleaseTemporary(blurredTex1);
RenderTexture.ReleaseTemporary(blurredTex2);
RenderTexture.ReleaseTemporary(compositeTex);
```

---

## Parameters Summary

| Parameter | Description | Tips |
|-----------|-------------|------|
| **Threshold** | Brightness cutoff | Higher = fewer glowing areas |
| **Intensity** | Brightness multiplier | Higher = stronger glow |
| **Attenuation** | Streak fade rate | Lower = faster fade, shorter streaks |
| **Divide** | Resolution divisor | Higher = better performance, softer result |
| **Iteration** | Blur passes per direction | 2-3 typical; more = longer streaks |
| **NumOfStreak** | Number of ray directions | 4 = classic cross, 6 = star of David |
| **AngleOfStreak** | Rotation offset | Adjust starting angle |
| **Color** | Tint color | Warm colors for sun, cool for artificial light |

---

## Performance Considerations

This effect is relatively expensive because:
1. Multiple shader passes per frame
2. Multiple streak directions, each with iterations
3. Fragment-heavy operations

**Optimization strategies:**
- Reduce resolution (`divide` parameter)
- Limit iteration count
- Reduce number of streaks
- Consider temporal spreading (alternate directions across frames)

---

## Variations and Extensions

Beyond the basic Kawase implementation:

- **Non-uniform streaks** - Vary parameters per iteration for organic, asymmetric rays
- **Animated parameters** - Use noise to vary parameters over time for shimmer
- **Threshold variation** - Different thresholds can create multiple glow layers
- **Colored streaks** - Different colors per direction for chromatic effects

For physically accurate light rays, you'd need approaches beyond post-processing (volumetric rendering, light shafts), but StarGlow provides an efficient, visually appealing approximation that works well for stylized rendering.

---

## Key Takeaways

1. **Brightness extraction** isolates pixels above threshold for processing
2. **Resolution reduction** is critical for performance in iterative effects
3. **Directional blur** with attenuation creates the streak effect
4. **Exponential sampling** (pow(4, iteration)) efficiently extends streak length
5. **Multiple directions** combined create the star pattern
6. **Flexible compositing** allows artistic control over final appearance

## References

- Frame Buffer Postprocessing Effects in DOUBLE-S.T.E.A.L (Wreckless) - Masaki Kawase, GDC 2003
  - http://www.daionet.gr.jp/~masa/archives/GDC2003_DSTEAL.ppt
