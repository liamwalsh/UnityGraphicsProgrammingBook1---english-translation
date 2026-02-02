# Chapter 8: Simple Soft Deformation

**Author: xjine**

## Introduction

When expressing the softness of objects, you might simulate springs, fluids, or soft bodies. This chapter presents a simpler approach - achieving soft, cartoon-like deformation without complex physics calculations.

The technique creates hand-drawn animation-style squash and stretch effects.

**Sample Project**: `OverReaction` in the Unity Graphics Programming 3 repository.

## Sample Scenes

### OverReaction Scene
Basic deformation testing. Move objects using the manipulator or Inspector to see deformation.

### PhysicsScene
Apply forces with arrow keys. Place multiple objects to observe context-dependent deformation.

## Deformation Rules

Three fundamental principles govern deformation:

1. **Larger changes = larger deformation**
2. **No change = deformation gradually returns to normal**
3. **Reversed direction = reversed deformation**

We focus on movement-based deformation, tracking movement direction and magnitude as "move energy" (expressed as a vector).

*Note: In proper physics terminology, kinetic energy is scalar. We use "move energy" as a directional quantity for this purpose.*

## Move Energy Calculation

### Why FixedUpdate?

```csharp
protected void FixedUpdate()
{
    this.crntMove = this.transform.position - this.prevPosition;

    UpdateMoveEnergy();
    UpdateDeformEnergy();
    DeformMesh();

    this.prevPosition = this.transform.position;
    this.prevMove = this.crntMove;
}
```

`FixedUpdate` runs at fixed time intervals, unlike `Update` which varies with frame rate. Benefits:
- Consistent behavior with Unity's PhysX physics
- No need for high-frequency mesh deformation
- Predictable calculations regardless of frame rate

### Component-Wise Energy Update

```csharp
protected void UpdateMoveEnergy()
{
    this.moveEnergy = new Vector3()
    {
        x = UpdateMoveEnergy(
            this.crntMove.x, this.prevMove.x, this.moveEnergy.x),
        y = UpdateMoveEnergy(
            this.crntMove.y, this.prevMove.y, this.moveEnergy.y),
        z = UpdateMoveEnergy(
            this.crntMove.z, this.prevMove.z, this.moveEnergy.z),
    };
}
```

Each axis is calculated independently.

### Sign Helper Function

```csharp
public static int Sign(float value)
{
    return value == 0 ? 0 : (value > 0 ? 1 : -1);
}
```

### Case 1: No Current Movement

When stationary, existing energy decays:

```csharp
protected float UpdateMoveEnergy(float crntMove, float prevMove, float moveEnergy)
{
    int crntMoveSign = Sign(crntMove);
    int prevMoveSign = Sign(prevMove);
    int moveEnergySign = Sign(moveEnergy);

    if (crntMoveSign == 0)
    {
        return moveEnergy * this.undeformPower;  // Decay
    }
    // ...
}
```

### Case 2: Direction Reversal (Current vs Previous)

When movement direction reverses, flip the energy:

```csharp
if (crntMoveSign != prevMoveSign)
{
    return moveEnergy - crntMove;
}
```

### Case 3: Energy Opposing Current Movement

When energy direction opposes movement, reduce energy:

```csharp
if (crntMoveSign != moveEnergySign)
{
    return moveEnergy + crntMove;
}
```

### Case 4: Same Direction

When energy and movement align, take the larger (decayed energy vs amplified movement):

```csharp
if (crntMoveSign < 0)
{
    return Mathf.Min(crntMove * this.deformPower,
                     moveEnergy * this.undeformPower);
}
else
{
    return Mathf.Max(crntMove * this.deformPower,
                     moveEnergy * this.undeformPower);
}
```

- `deformPower`: Amplifies new movement for visible effect
- `undeformPower`: Decays existing energy (< 1.0)

## Deform Energy Calculation

Deform energy determines actual mesh deformation, derived from move energy.

### Energy Transfer Based on Direction

Not all energy transfers to deformation when movement and energy directions differ:

```csharp
protected void UpdateDeformEnergy()
{
    float deformEnergyVertical
        = this.moveEnergy.magnitude
        * Vector3.Dot(this.moveEnergy.normalized,
                      this.crntMove.normalized);
    // ...
}
```

The dot product ranges:
- **1.0**: Perfectly aligned directions (full transfer)
- **0.0**: Perpendicular (no transfer)
- **-1.0**: Opposite directions (negative/compression)

### Horizontal Compensation

When an object stretches vertically, it should compress horizontally (and vice versa):

```csharp
float deformEnergyHorizontalRatio
    = deformEnergyVertical / this.maxDeformScale;

float deformEnergyHorizontal
    = 1 - deformEnergyHorizontalRatio;
```

If vertical deformation is +0.8 (stretch), horizontal becomes 1 - 0.8 = 0.2 (compress).

### Compression Case (Moving Into Energy)

When the object compresses in movement direction (negative dot product):

```csharp
if (deformEnergyVertical < 0)
{
    deformEnergyVertical = deformEnergyHorizontalRatio;
}

deformEnergyVertical = 1 + deformEnergyVertical;
```

This inverts the stretch/compress relationship.

### Complete UpdateDeformEnergy

```csharp
protected void UpdateDeformEnergy()
{
    float deformEnergyVertical
        = this.moveEnergy.magnitude
        * Vector3.Dot(this.moveEnergy.normalized,
                      this.crntMove.normalized);

    float deformEnergyHorizontalRatio
        = deformEnergyVertical / this.maxDeformScale;

    float deformEnergyHorizontal
        = 1 - deformEnergyHorizontalRatio;

    if (deformEnergyVertical < 0)
    {
        deformEnergyVertical = deformEnergyHorizontalRatio;
    }

    deformEnergyVertical = 1 + deformEnergyVertical;

    // Clamp to valid range
    deformEnergyVertical = Mathf.Clamp(deformEnergyVertical,
                                        this.minDeformScale,
                                        this.maxDeformScale);

    deformEnergyHorizontal = Mathf.Clamp(deformEnergyHorizontal,
                                          this.minDeformScale,
                                          this.maxDeformScale);

    this.deformEnergy = new Vector3(deformEnergyHorizontal,
                                    deformEnergyVertical,
                                    deformEnergyHorizontal);
}
```

## Mesh Deformation

The deform energy represents scaling along the movement direction. To apply it correctly, we need coordinate transformations.

*Note: For production, consider GPU-based shader implementation for better performance.*

### Required Matrices

```csharp
protected void DeformMesh()
{
    Vector3[] deformedVertices = new Vector3[this.baseVertices.Length];

    // Current object rotation and inverse
    Quaternion crntRotation = this.transform.localRotation;
    Quaternion crntRotationI = Quaternion.Inverse(crntRotation);

    // Move energy direction rotation and inverse
    Quaternion moveEnergyRotation
        = Quaternion.FromToRotation(Vector3.up, this.moveEnergy.normalized);
    Quaternion moveEnergyRotationI = Quaternion.Inverse(moveEnergyRotation);
    // ...
}
```

### Transformation Steps

1. Apply current rotation to unrotated base vertices
2. Apply inverse move-energy rotation (align to up axis)
3. Scale by deformEnergy
4. Apply move-energy rotation (restore direction)
5. Apply inverse current rotation (restore orientation)

```csharp
for (int i = 0; i < this.baseVertices.Length; i++)
{
    deformedVertices[i] = this.baseVertices[i];

    // 1. Apply current rotation
    deformedVertices[i] = crntRotation * deformedVertices[i];

    // 2. Rotate to align with up (for scaling)
    deformedVertices[i] = moveEnergyRotationI * deformedVertices[i];

    // 3. Apply deformation scaling
    deformedVertices[i] = new Vector3(
        deformedVertices[i].x * this.deformEnergy.x,
        deformedVertices[i].y * this.deformEnergy.y,
        deformedVertices[i].z * this.deformEnergy.z);

    // 4. Restore move-energy rotation
    deformedVertices[i] = moveEnergyRotation * deformedVertices[i];

    // 5. Remove current rotation
    deformedVertices[i] = crntRotationI * deformedVertices[i];
}

this.baseMesh.vertices = deformedVertices;
```

**Debugging Tip**: Comment out steps sequentially to understand each transformation's effect.

## Key Takeaways

1. **Move energy** tracks object velocity as a persisting, decaying vector
2. **Deform energy** converts move energy to actual scale values
3. **Dot product** determines energy transfer based on direction alignment
4. **Volume preservation**: Vertical stretch causes horizontal compression
5. **Coordinate transformations** align deformation with movement direction
6. **FixedUpdate** ensures consistent physics-friendly calculations
7. Simple implementation yields **significant visual impact**

## Possible Extensions

- Support for rotation-based deformation
- Scale change response
- Adjustable deformation center of mass
- Skinned mesh animation integration

## Visual Impact

Despite the simple implementation, the technique dramatically changes visual impression. The cartoon-like squash and stretch brings objects to life with minimal computational cost.
