# Chapter 7: Portal Implementation in Unity

**Author: Fuqunaga (@fuqunaga)**

## Introduction

Have you played Portal? Released by Valve in 2007, it's a legendary puzzle action game featuring portals - holes connected like wormholes that let you see through to the other side, allowing objects and the player to warp through.

Essentially, it's a "Anywhere Door" (Dokodemo Door). Using the portal gun, you can place portals on flat surfaces and navigate through them.

This chapter implements a simplified version of Portal's mechanics in Unity.

**Sample Project**: `PortalGateSystem` in the Unity Graphics Programming 3 repository.

## Project Overview

### Required Elements

- Movable player character
- Field/environment
- Portal gun (spawns portals at target location)

### Player Setup

The player needs a full body model (visible through portals) despite first-person view. The sample uses Unity's **Adam** character model.

To prevent the player model from appearing in the main camera (causing polygon clipping issues), a **Player** layer is created and excluded from the main camera's Culling Mask.

### Controls

- Movement: WASD or arrow keys (Shift for sprint)
- Look: Mouse movement
- Portal fire: Left/Right mouse click
- Ball fire: E key
- Jump: Space

### Field Setup

The sample uses **playGROWnd** assets from unity3d-jp for a Portal-like aesthetic. The field is a rectangular room with simplified collision using transparent colliders on each face (labeled **StageColl** layer).

Floor collision extends beyond the room to prevent objects from falling through after warping.

## Gate Generation

Gates are oriented in local space with XY plane as the surface and Z+ as the pass-through direction.

### Portal Gun Implementation

```csharp
void Shot(int idx)
{
    RaycastHit hit;
    if (Physics.Raycast(transform.position,
        transform.forward,
        out hit,
        float.MaxValue,
        LayerMask.GetMask(new[] { "StageColl" })))
    {
        var gate = gatePair[idx];
        if (gate == null)
        {
            var go = Instantiate(gatePrefab);
            gate = gatePair[idx] = go.GetComponent<PortalGate>();

            var pair = gatePair[(idx + 1) % 2];
            if (pair != null)
            {
                gate.SetPair(pair);
                pair.SetPair(gate);
            }
        }

        gate.hitColl = hit.collider;

        var trans = gate.transform;
        var normal = hit.normal;
        var up = normal.y >= 0f ? transform.up : transform.forward;

        trans.position = hit.point + normal * gatePosOffset;
        trans.rotation = Quaternion.LookRotation(-normal, up);

        gate.Open();
    }
}
```

Key points:
- Raycast only against **StageColl** layer
- Store hit collider in `PortalGate.hitColl` for later collision handling
- Position offset from surface to prevent z-fighting
- **Up vector handling**: When placing on ceiling, using `transform.up` causes disorienting 180-degree flips; using `transform.forward` maintains intuitive orientation

## Virtual Camera System

### The Core Challenge

When a gate opens, you need to see through to the paired gate's location. The solution: **render to a RenderTexture using a Virtual Camera positioned at the paired gate, then display that texture on the portal surface**.

### Camera Generation

```csharp
private void OnWillRenderObject()
{
    // Called per camera
    VirtualCamera pairVC;
    if (!pairVCTable.TryGetValue(cam, out pairVC))
    {
        if ((vc == null) || vc.generation < maxGeneration)
        {
            pairVC = pairVCTable[cam] = CreateVirtualCamera(cam, vc);
            return;
        }
    }
}
```

### Infinite Recursion Problem

When two gates face each other, they create a mirror effect - infinite recursion of gates within gates.

Each recursion level needs its own Virtual Camera:
1. Main camera sees Gate A → needs VirtualCamera1
2. VirtualCamera1 sees Gate B → needs VirtualCamera2
3. VirtualCamera2 sees Gate A → needs VirtualCamera3
4. And so on...

Solution: Limit recursion with `maxGeneration` and use previous frame's texture for deeper levels.

### Virtual Camera Creation

```csharp
VirtualCamera CreateVirtualCamera(Camera parentCam, VirtualCamera parentVC)
{
    var rootCam = parentVC?.rootCamera ?? parentCam;
    var generation = parentVC?.generation + 1 ?? 1;

    var go = Instantiate(virtualCameraPrefab);
    go.name = rootCam.name + "_virtual" + generation;
    go.transform.SetParent(transform);

    var vc = go.GetComponent<VirtualCamera>();
    vc.rootCamera = rootCam;
    vc.parentCamera = parentCam;
    vc.parentGate = this;
    vc.generation = generation;

    vc.Init();

    return vc;
}
```

### Camera Initialization

```csharp
public void Init()
{
    camera_.aspect = rootCamera.aspect;
    camera_.fieldOfView = rootCamera.fieldOfView;
    camera_.nearClipPlane = rootCamera.nearClipPlane;
    camera_.farClipPlane = rootCamera.farClipPlane;
    camera_.cullingMask |= LayerMask.GetMask(new[] { PlayerLayerName });
    camera_.depth = parentCamera.depth - 1;

    camera_.targetTexture = tex0;
}
```

Note: **Player** layer is included in VirtualCamera's culling mask (unlike main camera) since the player should be visible through portals.

Camera depth is set lower than parent to ensure rendering happens first.

### Camera Update

```csharp
private void LateUpdate()
{
    if (parentCamera == null)
    {
        Destroy(gameObject);
        return;
    }

    camera_.enabled = parentGate.IsVisible(parentCamera);
    if (camera_.enabled)
    {
        var parentCamTrans = parentCamera.transform;
        parentGate.UpdateTransformOnPair(
            transform,
            parentCamTrans.position,
            parentCamTrans.rotation
        );

        UpdateCamera();
    }
}
```

### Visibility Check

```csharp
public bool IsVisible(Camera camera)
{
    var pos = transform.position;
    var camPos = camera.transform.position;

    // Direction check: is gate facing camera?
    var camToGateDir = (pos - camPos).normalized;
    var dot = Vector3.Dot(camToGateDir, transform.forward);
    if (dot > 0f)
    {
        // Frustum culling
        var planes = GeometryUtility.CalculateFrustumPlanes(camera);
        return GeometryUtility.TestPlanesAABB(planes, coll.bounds);
    }

    return false;
}
```

Two checks:
1. **Facing check**: Dot product between camera-to-gate direction and gate forward
2. **Frustum check**: Unity's built-in frustum-AABB intersection test

### Transform Mapping to Paired Gate

```csharp
public void UpdateTransformOnPair(Transform trans, Vector3 worldPos, Quaternion worldRot)
{
    // Convert to local space of this gate
    var localPos = transform.InverseTransformPoint(worldPos);
    var localRot = Quaternion.Inverse(transform.rotation) * worldRot;

    var pairGateTrans = pair.transform;
    var gateRot = pair.gateRot;  // 180-degree Y rotation

    // Convert from paired gate's local space to world
    var pos = pairGateTrans.TransformPoint(gateRot * localPos);
    var rot = pairGateTrans.rotation * gateRot * localRot;

    trans.SetPositionAndRotation(pos, rot);
}
```

The `gateRot` (180-degree Y rotation) flips the coordinate system from gate front to back.

### Optimized Camera Parameters

```csharp
void UpdateCamera()
{
    var pair = parentGate.pair;
    var pairTrans = pair.transform;
    var mesh = pair.GetComponent<MeshFilter>().sharedMesh;
    var vtxList = mesh.vertices
        .Select(vtx => pairTrans.TransformPoint(vtx)).ToList();

    // Minimize frustum to gate bounds
    TargetCameraUtility.Update(camera_, vtxList);

    // Oblique near clip plane at gate surface
    var clipPlane = CalcPlane(camera_,
        pairGateTrans.position,
        -pairGateTrans.forward);

    camera_.projectionMatrix = camera_.CalculateObliqueMatrix(clipPlane);
}
```

Two optimizations:
1. **Frustum narrowing**: Only render what's visible through the gate
2. **Oblique near clip**: Don't render anything between camera and gate surface

## Gate Rendering

The shader handles multiple states:
- Frame and interior display
- Opening animation (expanding circle)
- Pre-pairing state (wavy background)
- Post-pairing fade to portal view
- Fallback to previous frame when beyond max generation

### Vertex Shader

```hlsl
v2f vert(appdata_img In)
{
    v2f o;

    float3 posWorld = mul(unity_ObjectToWorld, float4(In.vertex.xyz, 1)).xyz;
    float4 clipPos = mul(UNITY_MATRIX_VP, float4(posWorld, 1));
    float4 clipPosOnMain = mul(_MainCameraViewProj, float4(posWorld, 1));

    o.pos = clipPos;
    o.uv = In.texcoord;
    o.sposOnMain = ComputeScreenPos(clipPosOnMain);
    o.grabPos = ComputeGrabScreenPos(o.pos);
    return o;
}
```

Two screen positions:
- `clipPos`: Current camera (for rendering)
- `clipPosOnMain`: Main camera (for sampling VirtualCamera texture)

### Fragment Shader

```hlsl
// Circle mask
float2 uv = (In.uv.xy - 0.5) * 2;
float insideRate = (1 - length(uv)) * _OpenRate;

// Wavy background (pre-pairing)
float4 grabUV = In.grabPos;
float2 grabOffset = float2(
    snoise(float3(uv, _Time.y)),
    snoise(float3(uv, _Time.y + 10))
);
grabUV.xy += grabOffset * 0.3 * insideRate;
float4 bgColor = tex2Dproj(_BackgroundTexture, grabUV);

// Portal view (post-pairing)
float2 sUV = In.sposOnMain.xy / In.sposOnMain.w;
float4 sideColor = tex2D(_MainTex, sUV);

// Blend based on connection state
float4 col = lerp(bgColor, sideColor, _ConnectRate);

// Frame rendering
float frame = smoothstep(0, 0.1, insideRate);
float frameColorRate = 1 - abs(frame - 0.5) * 2;
float mixRate = saturate(grabOffset.x + grabOffset.y);
float3 frameColor = lerp(_FrameColor0, _FrameColor1, mixRate);
col.xyz = lerp(col.xyz, frameColor, frameColorRate);

col.a = frame;
```

## Object Warping

The **PortalObj** component handles warp-related behavior for any warping object.

### Collision Disabling

Gates have large trigger colliders extending front and back. When objects enter, collision with the gate's surface is disabled.

```csharp
private void OnTriggerStay(Collider other)
{
    var gate = other.GetComponent<PortalGate>();
    if ((gate != null) && !touchingGates.Contains(gate) && (gate.pair != null))
    {
        touchingGates.Add(gate);
        Physics.IgnoreCollision(gate.hitColl, collider_, true);
    }
}

private void OnTriggerExit(Collider other)
{
    var gate = other.GetComponent<PortalGate>();
    if (gate != null)
    {
        touchingGates.Remove(gate);
        Physics.IgnoreCollision(gate.hitColl, collider_, false);
    }
}
```

Note: Uses `OnTriggerStay` instead of `OnTriggerEnter` to handle cases where the object enters before the paired gate exists.

### Warp Detection

```csharp
private void Update()
{
    var passedGate = touchingGates.FirstOrDefault(gate =>
    {
        var posOnGate = gate.transform.InverseTransformPoint(center.position);
        return posOnGate.z > 0f;  // Behind the gate surface
    });

    if (passedGate != null)
    {
        PassGate(passedGate);
    }
}
```

### Warp Execution

```csharp
void PassGate(PortalGate gate)
{
    gate.UpdateTransformOnPair(transform);

    if (rigidbody_ != null)
    {
        rigidbody_.velocity = gate.UpdateDirOnPair(rigidbody_.velocity);
        rigidbody_.useGravity = false;
        ignoreGravityStartTime = Time.time;
    }

    if (fpController != null)
    {
        fpController.m_MoveDir = gate.UpdateDirOnPair(fpController.m_MoveDir);
        fpController.InitMouseLook();
    }
}
```

Gravity is temporarily disabled to prevent objects from oscillating between floor-to-floor portals.

## Known Limitations

### High-Speed Collision Issues

Fast-moving objects may collide with walls before the trigger system disables collision. The large trigger zone mitigates but doesn't fully solve this.

**Wanted**: Better method for disabling collision during the same physics frame as trigger entry.

### Missing Partial Object Rendering

True portal behavior shows half an object on each side during transition. Current implementation teleports instantly.

Proper solution would require:
- Duplicating objects during transition
- Applying forces from both sides
- Potentially interfering with physics solver internals

## Key Takeaways

1. **Virtual Cameras** render portal views to RenderTextures
2. **Generation limiting** prevents infinite recursion with facing portals
3. **Oblique projection** clips at the gate surface for efficiency
4. **Coordinate transformation** maps positions between paired gates
5. **Trigger-based collision** handling enables pass-through
6. **GrabPass** provides background for pre-paired portals
7. Real portal systems require **careful physics handling**

## Future Ideas

As rendering becomes more realistic, previously "unrealistic" ideas like portals gain new life. The "anywhere door" concept, long dismissed as fantasy, may prove surprisingly applicable to novel experiences.

## References

- Portal: http://www.thinkwithportals.com/
- Adam Character Pack: Unity Asset Store
- playGROWnd: https://github.com/unity3d-jp/playgrownd
- PostProcessingStack: https://github.com/Unity-Technologies/PostProcessing
