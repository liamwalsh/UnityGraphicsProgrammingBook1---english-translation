# Chapter 1: GPU-Based Space Colonization Algorithm

**Author:** Nakamura
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** SpaceColonization

---

## Introduction

This chapter introduces the Space Colonization Algorithm - a technique for generating branching structures that follow point clouds - along with its GPU implementation and an application example combining it with skinned animation.

![Skinned Animation Scene](nakamura/SkinnedAnimationScene.png)

### What is the Space Colonization Algorithm?

The Space Colonization Algorithm was developed by Adam et al. as a tree modeling technique. It generates branching structures from a given point cloud with these key characteristics:

- **Balanced distribution** - Branches spread out naturally without clustering too densely
- **Shape control** - Branch placement is determined by initial point cloud positioning, making shapes controllable
- **Simple parameters** - Branch density can be controlled with simple parameters

## Algorithm Overview

The algorithm consists of six main steps:

1. **Setup** - Initialization
2. **Search** - Find influencing Attractions
3. **Attract** - Pull branches toward Attractions
4. **Connect** - Generate new Nodes and connect to existing Nodes
5. **Remove** - Delete Attractions within kill distance
6. **Grow** - Node growth

### Step 1: Setup - Initialization

During initialization, prepare a point cloud as **Attractions** (seed points for branches). Place one or more **Nodes** (branch points) within this Attraction field. These initially placed Nodes become the starting points for branches.

In the diagrams:
- Circular points represent Attractions
- Square points represent Nodes

### Step 2: Search - Finding Nearest Nodes

For each Attraction, search for the nearest Node within the **influence distance**.

### Step 3: Attract - Determining Branch Direction

For each Node with Attractions within influence range:
1. Calculate the direction to extend the branch
2. Find the point at **growth distance** along that direction
3. This point becomes a **Candidate** for a new Node

### Step 4: Connect - Extending Branches

Generate new Nodes at Candidate positions and connect them to original Nodes with Edges, extending the branch structure.

### Step 5: Remove - Cleaning Up Attractions

Delete Attractions within the **kill distance** from any Node.

### Step 6: Grow - Node Growth

Grow the Nodes and return to Step 2.

---

## Implementation

### Resource Structures

The Space Colonization Algorithm requires four element types that can increase/decrease:

#### Attraction (Seed Points)

```csharp
public struct Attraction {
    public Vector3 position;  // Position
    public int nearest;       // Nearest Node index
    public uint found;        // Whether a nearby Node was found
    public uint active;       // Whether this Attraction is active (1=active, 0=deleted)
}
```

The `active` flag allows soft deletion of Attractions without actually removing them from the buffer.

#### Node (Branch Points)

```csharp
public struct Node {
    public Vector3 position;  // Position
    public float t;           // Growth rate (0.0 ~ 1.0)
    public float offset;      // Distance from Root (Node depth)
    public float mass;        // Mass
    public int from;          // Index of parent Node
    public uint active;       // Whether this Node is active
}
```

Nodes use two buffers:
1. Buffer for actual Node data
2. Object pool buffer managing indices of inactive Nodes (Append/ConsumeStructuredBuffer)

#### Candidate (New Node Candidates)

```csharp
public struct Candidate {
    public Vector3 position;  // Position
    public int node;          // Index of source Node
}
```

#### Edge (Connections Between Nodes)

```csharp
public struct Edge {
    public int a, b;  // Indices of the two connected Nodes
}
```

### ComputeShader Implementation

#### Setup Kernel

The Setup kernel initializes the object pool by filling it with indices and marking all Nodes as inactive:

```hlsl
void Setup (uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    uint count, stride;
    _Nodes.GetDimensions(count, stride);
    if (idx >= count)
        return;

    _NodesPoolAppend.Append(idx);

    Node n = _Nodes[idx];
    n.active = false;
    _Nodes[idx] = n;
}
```

#### Seed Kernel

The Seed kernel creates initial Nodes at specified positions:

```hlsl
void Seed (uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    uint count, stride;
    _Seeds.GetDimensions(count, stride);
    if (idx >= count)
        return;

    Node n;
    uint i = CreateNode(n);  // Get index from object pool

    n.position = _Seeds[idx];
    n.t = 1;
    n.offset = 0;
    n.from = -1;
    n.mass = lerp(_MassMin, _MassMax, nrand(id.xy));
    _Nodes[i] = n;
}
```

#### Search Kernel

The Search kernel finds the nearest Node within influence distance for each Attraction:

```hlsl
void Search (uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    // ... bounds checking ...

    Attraction attr = _Attractions[idx];
    attr.found = false;

    if (attr.active)
    {
        float min_dist = _InfluenceDistance;
        uint nearest = -1;

        // Loop through all Nodes
        for (uint i = 0; i < count; i++)
        {
            Node n = _Nodes[i];
            if (n.active)
            {
                float3 dir = attr.position - n.position;
                float d = length(dir);
                if (d < min_dist)
                {
                    min_dist = d;
                    nearest = i;
                    attr.found = true;
                    attr.nearest = nearest;
                }
            }
        }
        _Attractions[idx] = attr;
    }
}
```

#### Attract Kernel

The Attract kernel calculates candidate positions for new Nodes:

```hlsl
void Attract (uint3 id : SV_DispatchThreadID)
{
    // ... setup ...

    Node n = _Nodes[idx];

    if (n.active && n.t >= 1.0)
    {
        float3 dir = (0.0).xxx;
        uint counter = 0;

        // Accumulate direction vectors to all influencing Attractions
        for (uint i = 0; i < count; i++)
        {
            Attraction attr = _Attractions[i];
            if (attr.active && attr.found && attr.nearest == idx)
            {
                float3 dir2 = (attr.position - n.position);
                dir += normalize(dir2);
                counter++;
            }
        }

        if (counter > 0)
        {
            Candidate c;
            dir = dir / counter;  // Average direction
            c.position = n.position + (dir * _GrowthDistance);
            c.node = idx;
            _CandidatesAppend.Append(c);
        }
    }
}
```

#### Connect Kernel

The Connect kernel creates new Nodes from Candidates and connects them with Edges:

```hlsl
void Connect (uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    if (idx >= _ConnectCount)
        return;

    Candidate c = _CandidatesConsume.Consume();

    Node n1 = _Nodes[c.node];
    Node n2;

    uint idx2 = CreateNode(n2);
    n2.position = c.position;
    n2.offset = n1.offset + 1.0;  // Depth = parent + 1
    n2.from = c.node;
    n2.mass = lerp(_MassMin, _MassMax, nrand(float2(c.node, idx2)));

    _Nodes[c.node] = n1;
    _Nodes[idx2] = n2;

    CreateEdge(c.node, idx2);
}
```

#### Remove Kernel

The Remove kernel deactivates Attractions within kill distance of any Node:

```hlsl
void Remove(uint3 id : SV_DispatchThreadID)
{
    // ... setup ...

    Attraction attr = _Attractions[idx];
    if (!attr.active)
        return;

    for (uint i = 0; i < count; i++)
    {
        Node n = _Nodes[i];
        if (n.active)
        {
            float d = distance(attr.position, n.position);
            if (d < _KillDistance)
            {
                attr.active = false;
                _Attractions[idx] = attr;
                return;
            }
        }
    }
}
```

#### Grow Kernel

The Grow kernel increases the growth rate of each Node:

```hlsl
void Grow (uint3 id : SV_DispatchThreadID)
{
    // ... setup ...

    Node n = _Nodes[idx];
    if (n.active)
    {
        // Mass parameter randomizes growth speed per Node
        n.t = saturate(n.t + _DT * n.mass);
        _Nodes[idx] = n;
    }
}
```

---

## Rendering

### Line Topology Rendering

The simplest approach uses Line Mesh topology with GPU instancing:

```csharp
protected Mesh BuildSegment()
{
    var mesh = new Mesh();
    mesh.vertices = new Vector3[2] { Vector3.zero, Vector3.up };
    mesh.uv = new Vector2[2] { new Vector2(0f, 0f), new Vector2(0f, 1f) };
    mesh.SetIndices(new int[2] { 0, 1 }, MeshTopology.Lines, 0);
    return mesh;
}
```

The vertex shader controls Edge length based on Node growth rate:

```hlsl
v2f vert(appdata IN, uint iid : SV_InstanceID)
{
    v2f OUT;

    Edge e = _Edges[iid];
    Node a = _Nodes[e.a];
    Node b = _Nodes[e.b];

    float3 ap = a.position;
    float3 bp = b.position;
    float3 dir = bp - ap;

    // Scale edge length by Node b's growth rate
    bp = ap + normalize(dir) * length(dir) * b.t;

    // Vertex ID 0 = node a, 1 = node b
    float3 position = lerp(ap, bp, IN.vid);

    OUT.position = UnityObjectToClipPos(float4(position, 1));
    OUT.alpha = (a.active && b.active) && (iid < _EdgesCount);

    return OUT;
}
```

### Geometry Shader Rendering

For thicker lines, a Geometry Shader can convert Line segments to capsule shapes:

```hlsl
[maxvertexcount(64)]
void geom(line v2g IN[2], inout TriangleStream<g2f> OUT) {
    // Calculate tangent, normal, binormal
    float3 t = normalize(p1.position - p0.position);
    float3 n = normalize(p0.viewDir);
    float3 bn = cross(t, n);
    n = cross(t, bn);

    // Build capsule side
    for (uint i = 0; i < cols; i++) {
        float r = (i * cols_inv) * UNITY_TWO_PI;
        float s, c;
        sincos(r, s, c);
        float3 normal = normalize(n * c + bn * s);
        // ... emit vertices ...
    }

    // Build capsule end caps (hemispheres)
    // ... hemisphere construction ...
}
```

---

## Application: Skinned Animation Integration

This advanced application combines Space Colonization with skinned mesh animation, creating branches that follow animated model shapes.

### Overview

1. Prepare an animated model
2. Generate a point cloud filling the model's volume (Attractions)
3. Add Bone information to Attractions and Nodes
4. Apply Bone transformations (skinning) to Node positions

### Modified Structures

```csharp
public struct SkinnedAttraction {
    public Vector3 position;
    public int bone;      // Bone index
    public int nearest;
    public uint found;
    public uint active;
}

public struct SkinnedNode {
    public Vector3 position;
    public Vector3 animated;  // Position after skinning
    public int index0;        // Bone index
    public float t;
    public float offset;
    public float mass;
    public int from;
    public uint active;
}
```

### Volume Sampling

The VolumeSampler package generates point clouds filling mesh volumes using:
1. GPU-based voxelization (from Volume 2's "Real-Time GPU-Based Voxelizer")
2. Poisson Disk Sampling to create evenly distributed points

### Skinning Implementation

Each Node receives bone information from its nearest Attraction. The Animate kernel applies bone transformations:

```hlsl
void Animate (uint3 id : SV_DispatchThreadID)
{
    // ... setup ...

    SkinnedNode node = _Nodes[idx];
    if (node.active)
    {
        // Apply skinning transformation
        float4x4 bind = _BindPoses[node.index0];
        float4x4 m = _BoneMatrices[node.index0];
        node.animated = mul(mul(m, bind), float4(node.position, 1)).xyz;
        _Nodes[idx] = node;
    }
}
```

The rendering shader then uses the `animated` position instead of the original `position`.

---

## Key Parameters

The algorithm is controlled by three main parameters:

| Parameter | Description | Effect |
|-----------|-------------|--------|
| **Influence Distance** | Range within which Attractions affect Nodes | Larger = more widely spread branches |
| **Growth Distance** | Length each branch segment grows | Larger = longer segments, sparser structure |
| **Kill Distance** | Range within which Attractions are removed | Larger = fewer, thicker branches |

### Advanced Variations

- **Localized parameters** - Vary parameters spatially for different densities in different areas
- **Time-varying parameters** - Animate parameters for evolving structures
- **Dynamic Attractions** - Add Attractions during execution for more complex patterns

---

## Key Takeaways

1. **Space Colonization** generates organic branching structures from point clouds
2. **GPU implementation** enables real-time generation using Append/Consume buffers
3. **Object pooling** on GPU manages dynamic Node/Edge creation
4. **Three key parameters** (influence, growth, kill distance) control branch density
5. **Skinned animation integration** allows branches to follow animated meshes
6. **Geometry Shader rendering** creates volumetric branch visualization

## References

- Modeling Trees with a Space Colonization Algorithm - http://algorithmicbotany.org/papers/colonization.egwnp2007.large.pdf
- Algorithmic Design with Houdini - https://vimeo.com/305061631#t=1500s
