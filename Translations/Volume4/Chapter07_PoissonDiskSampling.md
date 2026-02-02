# Chapter 7: Poisson Disk Sampling

**Author:** Aoyama
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** (Poisson Disk Sampling implementation)

---

## Introduction

This chapter explains **Poisson Disk Sampling (PDS)** and its CPU implementation using the **Fast Poisson Disk Sampling in Arbitrary Dimensions (FPDS)** algorithm.

FPDS was proposed by Robert Bridson of the University of British Columbia in a SIGGRAPH 2007 paper. Its key innovation is dimension-independence - the same algorithm works in any number of dimensions.

---

## What is Poisson Disk Sampling?

Consider plotting many points in a space with a minimum distance constraint **d** (where d > 0). A **Poisson-disk distribution** is a set of randomly positioned points where **every pair of points is separated by at least distance d**.

Key property: No matter which two points you select from the distribution, their distance will never be less than d.

**PDS** is the process of generating (sampling) such a distribution computationally.

### Applications

PDS produces uniformly distributed random point sets, useful for:
- **Anti-aliasing** - Sample positions for filtering
- **Texture synthesis** - Selecting blend pixels
- **Object placement** - Natural-looking random distributions
- **Image effects** - Blur kernels, dithering
- **Procedural generation** - Vegetation, crowd placement

---

## Fast Poisson Disk Sampling Algorithm

### Key Advantages of FPDS

- **Dimension-independent** - Same algorithm for 2D, 3D, or any dimension
- **O(N) time complexity** - Linear in the number of output points
- **Simple implementation** - Uses basic data structures

Previous PDS methods were typically limited to 2D. FPDS efficiently handles any dimension.

### Parameters

| Parameter | Symbol | Description |
|-----------|--------|-------------|
| Sampling space | R^n | Bounding region for points |
| Minimum distance | r | Required separation between points |
| Sampling limit | k | Attempts per active point |

---

## Algorithm Steps

### Step 1: Divide Space into Grid

To accelerate distance checks, divide the sampling space into a grid of cells.

**Cell size:** r / sqrt(n)

Where n is the number of dimensions. For 3D: r / sqrt(3)

**Why this size?**
- sqrt(n) is the diagonal length of an n-dimensional unit hypercube
- With this cell size, each cell can contain **at most one point**
- Checking neighbors requires only looking at cells within +-n in each dimension

```csharp
// Create 3D grid for sampling
Vector3?[,,] GetGrid(Vector3 bottomLeftBack, Vector3 topRightForward,
    float min, int iteration)
{
    var dimension = (topRightForward - bottomLeftBack);
    var cell = min * InvertRootTwo;  // r / sqrt(2) for 3D

    return new Vector3?[
        Mathf.CeilToInt(dimension.x / cell) + 1,
        Mathf.CeilToInt(dimension.y / cell) + 1,
        Mathf.CeilToInt(dimension.z / cell) + 1
    ];
}

// Get grid cell index for a point
Vector3Int GetGridIndex(Vector3 point, Settings set)
{
    return new Vector3Int(
        Mathf.FloorToInt((point.x - set.BottomLeftBack.x) / set.CellSize),
        Mathf.FloorToInt((point.y - set.BottomLeftBack.y) / set.CellSize),
        Mathf.FloorToInt((point.z - set.BottomLeftBack.z) / set.CellSize)
    );
}
```

### Step 2: Create Initial Sample Point

Generate a completely random point within the sampling space. This is valid since no other points exist yet.

Add this point to:
1. Its corresponding grid cell
2. The **sample list** (final output)
3. The **active list** (points to expand from)

```csharp
void GetFirstPoint(Settings set, Bags bags)
{
    var first = new Vector3(
        Random.Range(set.BottomLeftBack.x, set.TopRightForward.x),
        Random.Range(set.BottomLeftBack.y, set.TopRightForward.y),
        Random.Range(set.BottomLeftBack.z, set.TopRightForward.z)
    );
    var index = GetGridIndex(first, set);

    bags.Grid[index.x, index.y, index.z] = first;
    bags.SamplePoints.Add(first);
    bags.ActivePoints.Add(first);
}
```

### Step 3: Select Base Point from Active List

Randomly select an index i from the active list. The point at this index, x_i, becomes the center for generating new samples.

```csharp
var index = Random.Range(0, bags.ActivePoints.Count);
var point = bags.ActivePoints[index];
```

### Step 4: Sample New Points

From x_i, generate a random point x'_i in the spherical shell between radius r and 2r.

**Why r to 2r?**
- Less than r: Would violate minimum distance from x_i
- Greater than 2r: Leaves gaps that could contain valid points

Check if x'_i is valid:
1. Is it within the sampling bounds?
2. Is it at least distance r from all nearby points?

For the distance check, only examine cells within +-3 of x'_i's cell (covers all cells within distance r).

```csharp
private static bool GetNextPoint(Vector3 point, Settings set, Bags bags)
{
    // Generate random point in spherical shell [r, 2r]
    var p = point + GetRandPosInSphere(set.MinimumDistance, 2f * set.MinimumDistance);

    // Check bounds
    if (!set.Dimension.Contains(p)) { return false; }

    var minimum = set.MinimumDistance * set.MinimumDistance;
    var index = GetGridIndex(p, set);
    var drop = false;

    // Calculate neighbor search range
    var around = 3;
    var fieldMin = new Vector3Int(
        Mathf.Max(0, index.x - around),
        Mathf.Max(0, index.y - around),
        Mathf.Max(0, index.z - around)
    );
    var fieldMax = new Vector3Int(
        Mathf.Min(set.GridWidth, index.x + around),
        Mathf.Min(set.GridHeight, index.y + around),
        Mathf.Min(set.GridDepth, index.z + around)
    );

    // Check neighboring cells for conflicts
    for (var i = fieldMin.x; i <= fieldMax.x && !drop; i++)
    {
        for (var j = fieldMin.y; j <= fieldMax.y && !drop; j++)
        {
            for (var k = fieldMin.z; k <= fieldMax.z && !drop; k++)
            {
                var q = bags.Grid[i, j, k];
                if (q.HasValue && (q.Value - p).sqrMagnitude <= minimum)
                {
                    drop = true;  // Too close to existing point
                }
            }
        }
    }

    if (drop) { return false; }

    // Point is valid - add to all data structures
    bags.SamplePoints.Add(p);
    bags.ActivePoints.Add(p);
    bags.Grid[index.x, index.y, index.z] = p;
    return true;
}
```

### Step 5: Repeat Sampling

For each active point x_i, attempt k samples (Step 4).

If all k attempts fail (no valid points found), remove x_i from the active list - this area is fully sampled.

Return to Step 3 and select a new active point.

Continue until the active list is empty.

```csharp
public static List<Vector3> Sampling(Vector3 bottomLeft, Vector3 topRight,
    float minimumDistance, int iterationPerPoint)
{
    var settings = GetSettings(
        bottomLeft, topRight, minimumDistance,
        iterationPerPoint <= 0 ? DefaultIterationPerPoint : iterationPerPoint
    );

    var bags = new Bags()
    {
        Grid = new Vector3?[
            settings.GridWidth + 1,
            settings.GridHeight + 1,
            settings.GridDepth + 1
        ],
        SamplePoints = new List<Vector3>(),
        ActivePoints = new List<Vector3>()
    };

    GetFirstPoint(settings, bags);

    do
    {
        var index = Random.Range(0, bags.ActivePoints.Count);
        var point = bags.ActivePoints[index];

        var found = false;
        for (var k = 0; k < settings.IterationPerPoint; k++)
        {
            found = found | GetNextPoint(point, settings, bags);
        }

        if (!found) { bags.ActivePoints.RemoveAt(index); }
    }
    while (bags.ActivePoints.Count > 0);

    return bags.SamplePoints;
}
```

---

## Algorithm Summary

1. **Divide space into grid** - Cell size r/sqrt(n) ensures one point per cell
2. **Place initial random point** - Add to grid, sample list, and active list
3. **Select active point** - Random selection from active list
4. **Attempt k samples** - Generate points in [r, 2r] shell, validate against neighbors
5. **Remove exhausted points** - When k attempts yield no valid points
6. **Repeat until done** - Active list empty = space fully sampled

---

## Complexity Analysis

| Operation | Complexity |
|-----------|------------|
| Grid lookup | O(1) |
| Neighbor check | O(1) - fixed number of cells |
| Per-point sampling | O(k) - k attempts |
| Total | O(N) - linear in output points |

The grid structure is key - it transforms O(N) distance checks into O(1) neighbor lookups.

---

## Visualization

The result can be visualized by placing circles/spheres at sampled positions:
- No circles overlap (minimum distance maintained)
- Space is filled as completely as possible
- Distribution appears random but uniform

---

## Parameter Guidelines

| Parameter | Effect of Increasing |
|-----------|---------------------|
| **Space (R^n)** | More points sampled (larger area) |
| **Distance (r)** | Fewer, more spread points |
| **Attempts (k)** | Better space filling, longer compute |

Typical k values: 20-30 for good results without excessive computation.

---

## Limitations

The basic FPDS algorithm:
- **Not parallelized** - Sequential active list processing
- **CPU-bound** - Large spaces or small r means significant computation time
- **Memory** - Grid storage grows with space size / cell size

For real-time applications, consider:
- Pre-computing samples
- GPU-accelerated variants
- Hierarchical approaches for large spaces

---

## Key Takeaways

1. **Poisson Disk Sampling** produces uniformly distributed random points with minimum separation
2. **Grid acceleration** reduces distance checks from O(N) to O(1)
3. **Cell size r/sqrt(n)** guarantees at most one point per cell
4. **Spherical shell [r, 2r]** sampling ensures valid candidates
5. **Active list** tracks expansion frontier; removal when exhausted
6. **Dimension-independent** - Same algorithm for 2D, 3D, nD
7. **O(N) complexity** makes it practical for moderate point counts
8. **Wide applications** - Anti-aliasing, object placement, procedural generation

## Applications in Graphics

- **Sampling patterns** for ray tracing, ambient occlusion
- **Object scattering** for vegetation, debris, crowds
- **Stippling** and artistic rendering
- **Noise generation** with better spectral properties than white noise
- **Level-of-detail** point selection

## References

- Fast Poisson Disk Sampling in Arbitrary Dimensions - Robert Bridson, SIGGRAPH 2007
  - https://www.cct.lsu.edu/~fharhad/ganbatte/siggraph2007/CD2/content/sketches/0250.pdf
