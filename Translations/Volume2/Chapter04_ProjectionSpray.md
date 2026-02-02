# Chapter 4: Projection Spray - Custom Lighting and Texture Painting

**Author**: Sugino Hironori

**Sample Project**: "ProjectionSpray" folder in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter covers implementing custom lighting without Unity's built-in lights, then applies those techniques to create a real-time spray painting system for 3D objects. The concept is to learn from Unity's built-in shaders (UnityCG.cginc) and apply that knowledge to create new functionality.

---

## Part 1: Implementing Custom Lights

### Understanding Unity's Built-in Lighting

Unity's built-in shaders can be downloaded from the Unity Download Archive. Key lighting files include:

- `CGIncludes/UnityCG.cginc`
- `CGIncludes/AutoLight.cginc`
- `CGIncludes/Lighting.cginc`
- `CGIncludes/UnityLightingCommon.cginc`

### Lambert Lighting Basics

The fundamental diffuse calculation in `Lighting.cginc`:

```hlsl
struct SurfaceOutput {
    fixed3 Albedo;
    fixed3 Normal;
    fixed3 Emission;
    half Specular;
    fixed Gloss;
    fixed Alpha;
};

inline fixed4 UnityLambertLight (SurfaceOutput s, UnityLight light)
{
    fixed diff = max(0, dot(s.Normal, light.dir));
    fixed4 c;
    c.rgb = s.Albedo * light.color * diff;
    c.a = s.Alpha;
    return c;
}
```

The key calculation: `dot(s.Normal, light.dir)` - the dot product between surface normal and light direction determines brightness.

### Visualizing Mesh Normals

Before implementing lighting, understanding normal visualization helps (Scene: `00_viewNormal.unity`):

```hlsl
v2f vert (appdata v)
{
    v2f o;
    o.vertex = UnityObjectToClipPos(v.vertex);
    o.normal = UnityObjectToWorldNormal(v.normal);
    return o;
}

half4 frag (v2f i) : SV_Target
{
    fixed4 col = half4(i.normal, 1);
    return col;
}
```

**Built-in Helper Functions**:
- `UnityObjectToClipPos` - transforms vertex from object to clip space
- `UnityObjectToWorldNormal` - transforms normal from object to world space

---

## Part 2: Point Light Implementation

**Scene**: `01_pointLight.unity`

### C# Component

```csharp
[ExecuteInEditMode]
public class PointLightComponent : MonoBehaviour
{
    static MaterialPropertyBlock mpb;
    public Renderer targetRenderer;
    public float intensity = 1f;
    public Color color = Color.white;

    void Update()
    {
        if (targetRenderer == null) return;
        if (mpb == null) mpb = new MaterialPropertyBlock();

        targetRenderer.GetPropertyBlock(mpb);
        mpb.SetVector("_LitPos", transform.position);
        mpb.SetFloat("_Intensity", intensity);
        mpb.SetColor("_LitCol", color);
        targetRenderer.SetPropertyBlock(mpb);
    }
}
```

### Shader Implementation

```hlsl
fixed4 frag (v2f i) : SV_Target
{
    half3 to = i.worldPos - _LitPos;
    half3 lightDir = normalize(to);
    half dist = length(to);
    half atten = _Intensity * dot(-lightDir, i.normal) / (dist * dist);
    half4 col = max(0.0, atten) * _LitCol;
    return col;
}
```

**Key Formula**: Intensity follows inverse square law: `intensity / (distance * distance)`

---

## Part 3: Spotlight Implementation

**Scene**: `02_spotLight.unity`

### Added Parameters

Spotlights require additional information:
- Light direction (via `worldToLocalMatrix`)
- Projection matrix (field of view, range)
- Light cookie texture

```csharp
var projMatrix = Matrix4x4.Perspective(angle, 1f, 0f, range);
var worldToLightMatrix = transform.worldToLocalMatrix;

mpb.SetMatrix("_WorldToLitMatrix", worldToLightMatrix);
mpb.SetMatrix("_ProjMatrix", projMatrix);
mpb.SetTexture("_Cookie", cookie);
```

### Spotlight Shader

```hlsl
fixed4 frag (v2f i) : SV_Target
{
    // Diffuse calculation
    half3 to = i.worldPos - _LitPos.xyz;
    half3 lightDir = normalize(to);
    half dist = length(to);
    half atten = _Intensity * dot(-lightDir, i.normal) / (dist * dist);

    // Spotlight cone calculation
    half4 lightSpacePos = mul(_WorldToLitMatrix, half4(i.worldPos, 1.0));
    half4 projPos = mul(_ProjMatrix, lightSpacePos);
    projPos.z *= -1;
    half2 litUv = projPos.xy / projPos.z;
    litUv = litUv * 0.5 + 0.5;

    // Cookie sampling and bounds check
    half lightCookie = tex2D(_Cookie, litUv);
    lightCookie *= 0 < litUv.x && litUv.x < 1 &&
                   0 < litUv.y && litUv.y < 1 && 0 < projPos.z;

    half4 col = max(0.0, atten) * _LitCol * lightCookie;
    return col;
}
```

---

## Part 4: Shadow Implementation

**Scene**: `03_spotLight-withShadow.unity`

### Creating a Depth Texture

Shadows require comparing depths from the light's perspective:

```csharp
depthOutput = new RenderTexture(
    shadowMapResolution, shadowMapResolution, 16, RenderTextureFormat.RFloat);
depthOutput.wrapMode = TextureWrapMode.Clamp;
_c.targetTexture = depthOutput;
_c.SetReplacementShader(depthRenderShader, "RenderType");
```

### Depth Render Shader

```hlsl
v2f vert (float4 pos : POSITION)
{
    v2f o;
    o.vertex = UnityObjectToClipPos(pos);
    o.depth = abs(UnityObjectToViewPos(pos).z);
    return o;
}

float frag (v2f i) : SV_Target
{
    return i.depth;
}
```

### Shadow Comparison

```hlsl
// Shadow calculation
half lightDepth = tex2D(_LitDepth, litUv).r;
atten *= 1.0 - saturate(10 * abs(lightSpacePos.z) - 10 * lightDepth);
```

If `lightSpacePos.z > lightDepth`, the surface is in shadow.

---

## Part 5: Projection Spray System

Now we apply the spotlight technique to paint on 3D objects in real-time.

### UV2 for Lightmap Painting

Objects without UV data can use Unity's "Generate Lightmap UVs" option to create a second UV channel (`TEXCOORD1`) suitable for painting.

### UV2 Expansion Shader

The key insight: we can render the mesh "unfolded" into UV2 space:

```hlsl
v2f vert(appdata v)
{
#if UNITY_UV_STARTS_AT_TOP
    v.uv2.y = 1.0 - v.uv2.y;
#endif
    float4 pos0 = UnityObjectToClipPos(v.vertex);
    float4 pos1 = float4(v.uv2 * 2.0 - 1.0, 0.0, 1.0);

    v2f o;
    o.vertex = lerp(pos0, pos1, _T);  // Animate between 3D and UV2
    o.uv2 = v.uv2;
    o.worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
    o.normal = UnityObjectToWorldNormal(v.normal);
    return o;
}
```

### The Spray System Architecture

**Components**:

1. **ProjectionSpray.cs** - Spray position/settings
2. **Drawable.cs** - Object being painted (holds RenderTexture)
3. **DrawableController.cs** - Orchestrates the painting

### Drawable Component (Ping-Pong Buffer)

```csharp
public void Draw(Material drawingMat)
{
    drawingMat.SetTexture("_MainTex", pingPongRts[0]);

    var currentActive = RenderTexture.active;
    RenderTexture.active = pingPongRts[1];
    GL.Clear(true, true, Color.clear);
    drawingMat.SetPass(0);
    Graphics.DrawMeshNow(mesh, transform.localToWorldMatrix);
    RenderTexture.active = currentActive;

    Swap(pingPongRts);

    // Optional crack filling
    if(fillCrack != null)
    {
        Graphics.Blit(pingPongRts[0], pingPongRts[1], fillCrack);
        Swap(pingPongRts);
    }

    Graphics.CopyTexture(pingPongRts[0], output);
}
```

### Spray Painting Shader

```hlsl
half4 frag (v2f i) : SV_Target
{
    // Spotlight-style calculations for spray intensity
    half3 to = i.worldPos - _DrawerPos.xyz;
    half3 dir = normalize(to);
    half dist = length(to);
    half atten = _Emission * dot(-dir, i.normal) / (dist * dist);

    // Cookie and shadow sampling (same as spotlight)
    // ...

    // Blend existing color with spray color
    i.uv.y = 1 - i.uv.y;
    half4 col = tex2D(_MainTex, i.uv);
    col.rgb = lerp(col.rgb, _Color.rgb,
                   saturate(col.a * _Emission * atten * cookie));
    col.a = 1;
    return col;
}
```

---

## Key Takeaways

| Concept | Application |
|---------|-------------|
| Lambert Lighting | `dot(normal, lightDir)` for basic diffuse |
| Inverse Square Law | `intensity / (distance * distance)` for realistic falloff |
| Projection Matrix | Converts world space to light/projector space |
| Depth Comparison | Enables shadow mapping |
| UV2 Space Rendering | Allows texture painting via mesh rendering |
| Ping-Pong Buffers | Enables iterative texture updates |

---

## Summary

By studying Unity's built-in shader includes (`UnityCG.cginc`, `Lighting.cginc`), we can:

1. Understand how Unity implements standard lighting
2. Create custom light systems independent of Unity's lighting
3. Repurpose lighting math for creative applications like texture painting

The projection spray technique demonstrates how understanding fundamentals opens doors to novel applications - the same math that creates spotlight shadows can paint colors onto 3D surfaces in real-time.

---

## References

- Unity Built-in Shader Helper Functions: [Unity Manual](https://docs.unity3d.com/Manual/SL-BuiltinFunctions.html)
- Coordinate transformation details: Unity Graphics Programming vol.1, Chapter 9 "Multi Plane Perspective Projection"
