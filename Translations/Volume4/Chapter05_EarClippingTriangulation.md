# Chapter 5: Triangulation by Ear Clipping

**Author:** Kaiware007
**Repository:** https://github.com/IndieVisualLab/UnityGraphicsProgramming4
**Sample Project:** TriangulationByEarClipping

---

## Introduction

This chapter explains **Ear Clipping** - a method for triangulating polygons. We cover not only simple polygon triangulation but also polygons with holes and hierarchically nested polygons.

### Sample Instructions

Run the DrawTest scene:
- **Left click** - Place vertices to create a polygon (avoid self-intersecting lines)
- **Right click** - Triangulate and generate mesh
- Create polygons inside existing meshes to create holes

---

## Simple Polygon Triangulation

A **simple polygon** is a closed polygon whose edges do not self-intersect.

**Key property:** Any simple polygon with n vertices can be triangulated into exactly n-2 triangles.

---

## The Ear Clipping Algorithm

Among various triangulation methods, Ear Clipping is notable for its simplicity. It's based on the **Two Ears Theorem**:

> Any simple polygon with 4 or more vertices has at least two "ears."

### What is an "Ear"?

An **ear** is a triangle where:
- Two edges are edges of the polygon
- The third edge lies entirely inside the polygon

### Algorithm Concept

Ear Clipping works by:
1. Finding ears in the polygon
2. Removing (clipping) them one at a time
3. Repeating until only one triangle remains

**Note:** While simple to implement, Ear Clipping is slower than other algorithms and may not be suitable for performance-critical applications.

---

## Triangulation Process

### Ear Detection Criteria

A vertex v_i is an ear if:

1. **Convex vertex** - The interior angle at v_i (formed by v_{i-1}, v_i, v_{i+1}) is less than 180 degrees
2. **No contained vertices** - The triangle formed by v_{i-1}, v_i, v_{i+1} contains no other polygon vertices

### Step-by-Step Process

**Initialization:**
1. Check all vertices against ear criteria
2. Add qualifying vertices to the ear list

**Main Loop:**
1. Take the first vertex from the ear list
2. Create a triangle with its neighboring vertices
3. Remove the vertex from the polygon
4. Re-check the two neighboring vertices (they may become ears or stop being ears)
5. Repeat until only 3 vertices remain (final triangle)

### Worked Example

Consider a 7-vertex polygon (vertices 0-6):

**Initial state:**
- Ear list: [0, 1, 4, 6]
- Vertex 2, 5: Not convex (angle >= 180)
- Vertex 3: Triangle (2,3,4) contains vertex 5

**Iteration 1:** Remove vertex 0
- Create triangle (6, 0, 1)
- Vertices 1, 6 remain ears
- Ear list: [1, 4, 6]

**Iteration 2:** Remove vertex 1
- Create triangle (6, 1, 2)
- Vertex 2 becomes convex and is now an ear
- Ear list: [4, 6, 2]

**Iteration 3:** Remove vertex 4
- Create triangle (3, 4, 5)
- Vertices 3, 5 become ears
- Ear list: [6, 2, 3, 5]

**Continue until complete...**

---

## Polygons with Holes

Ear Clipping doesn't directly handle holes, but we can work around this by **connecting** the outer polygon to inner polygons, creating a single simple polygon.

### Connection Process

**Prerequisites:**
- Outer polygon vertices are ordered clockwise
- Inner polygon vertices are ordered counter-clockwise

**Steps:**

1. **Find rightmost inner vertex** - Among all hole polygons, find the one with the maximum X-coordinate vertex. Call this vertex M.

2. **Cast ray right** - From M, cast a ray in the +X direction.

3. **Find intersection** - Find where this ray intersects the outer polygon. Call the intersection point I and the edge endpoints.

4. **Select connection point P** - Choose the edge endpoint with the larger X-coordinate.

5. **Check for obstructions** - Form triangle M-I-P. If no other vertices are inside this triangle, P is the connection point. If vertices exist inside, choose the one with the smallest angle to line M-I.

6. **Connect polygons** - Insert the inner polygon's vertices into the outer polygon at point P, duplicating M and P to create the "bridge."

7. **Repeat** - Process remaining holes from right to left.

### Why Right to Left?

Processing holes from rightmost to leftmost ensures that previously created bridges don't interfere with subsequent connections.

---

## Nested Polygon Hierarchies

For polygons containing polygons (like islands within lakes within land), we need to establish parent-child relationships.

### Building the Hierarchy

1. **Sort by bounding box area** - Larger polygons are potential parents
2. **Build tree recursively** - For each polygon, check if it's contained within another
3. **Alternate winding** - Odd depth = counter-clockwise (outer), even depth = clockwise (inner)
4. **Create dummy root** - Handle multiple independent polygons

### Processing the Hierarchy

1. Take a polygon from the tree (this is the outer polygon)
2. Get all immediate children (these are holes)
3. Connect and triangulate as a polygon-with-holes
4. Repeat for remaining polygons in the tree

---

## Implementation

### Data Structures

```csharp
// Vertex coordinates
List<Vector3> vertices = new List<Vector3>();

// Vertex indices (circular linked list behavior)
LinkedList<int> indices = new LinkedList<int>();

// Current ear vertices
List<int> earTipList = new List<int>();
```

### Polygon Class

```csharp
public class Polygon
{
    public enum LoopType
    {
        CW,   // Clockwise
        CCW,  // Counter-clockwise
        ERR,  // Undefined
    }

    public Vector3[] vertices;
    public LoopType loopType;
    // Methods for containment testing, etc.
}
```

### Tree Structure

```csharp
public class TreeNode<T>
{
    public TreeNode<T> parent = null;
    public List<TreeNode<T>> children = new List<TreeNode<T>>();
    public T Value;
    public bool isValue = false;  // False for dummy root

    public void AddChild(T val) { ... }
    public void RemoveChild(TreeNode<T> tree) { ... }
}
```

### Ear Checking

```csharp
void CheckVertex(LinkedListNode<int> node)
{
    int prevIndex = (node.Previous == null) ?
                    indices.Last.Value : node.Previous.Value;
    int nextIndex = (node.Next == null) ?
                    indices.First.Value : node.Next.Value;
    int nowIndex = node.Value;

    Vector3 prevVertex = vertices[prevIndex];
    Vector3 nextVertex = vertices[nextIndex];
    Vector3 nowVertex = vertices[nowIndex];

    bool isEar = false;

    // Check if interior angle < 180 degrees
    if (GeomUtil.IsAngleLessPI(nowVertex, prevVertex, nextVertex))
    {
        isEar = true;

        // Check no other vertices inside triangle
        foreach (int i in indices)
        {
            if (i == prevIndex || i == nowIndex || i == nextIndex)
                continue;

            Vector3 p = vertices[i];

            // Skip duplicated vertices (from hole connections)
            if (Vector3.Distance(p, prevVertex) <= float.Epsilon) continue;
            if (Vector3.Distance(p, nowVertex) <= float.Epsilon) continue;
            if (Vector3.Distance(p, nextVertex) <= float.Epsilon) continue;

            if (GeomUtil.IsInTriangle(p, prevVertex, nowVertex, nextVertex) <= 0)
            {
                isEar = false;
                break;
            }
        }

        // Update ear list
        if (isEar && !earTipList.Contains(nowIndex))
            earTipList.Add(nowIndex);
        else if (!isEar && earTipList.Contains(nowIndex))
            earTipList.Remove(nowIndex);
    }
}
```

### Point-in-Triangle Test

```csharp
// Check which side of a line a point is on
public static int CheckLine(Vector3 o, Vector3 p1, Vector3 p2)
{
    double x0 = o.x - p1.x;
    double y0 = o.y - p1.y;
    double x1 = p2.x - p1.x;
    double y1 = p2.y - p1.y;

    double det = x0 * y1 - x1 * y0;  // 2D cross product

    return (det > 0 ? +1 : (det < 0 ? -1 : 0));
    // +1 = right, -1 = left, 0 = on line
}

// Triangle containment (clockwise winding)
public static int IsInTriangle(Vector3 o, Vector3 p1, Vector3 p2, Vector3 p3)
{
    int sign1 = CheckLine(o, p2, p3);
    if (sign1 < 0) return +1;  // Outside

    int sign2 = CheckLine(o, p3, p1);
    if (sign2 < 0) return +1;

    int sign3 = CheckLine(o, p1, p2);
    if (sign3 < 0) return +1;

    // All same sign = inside, any zero = on edge
    return ((sign1 != 0 && sign2 != 0 && sign3 != 0) ? -1 : 0);
}
```

### Main Ear Clipping Loop

```csharp
void EarClipping()
{
    while (earTipList.Count > 0)
    {
        int nowIndex = earTipList[0];  // Take first ear

        LinkedListNode<int> indexNode = indices.Find(nowIndex);
        if (indexNode != null)
        {
            int prevIndex = (indexNode.Previous == null) ?
                            indices.Last.Value : indexNode.Previous.Value;
            int nextIndex = (indexNode.Next == null) ?
                            indices.First.Value : indexNode.Next.Value;

            // Create triangle
            resultTriangulation.Add(prevIndex);
            resultTriangulation.Add(nowIndex);
            resultTriangulation.Add(nextIndex);

            if (indices.Count == 3)
                break;  // Final triangle

            // Remove ear vertex
            earTipList.RemoveAt(0);
            indices.Remove(nowIndex);

            // Re-check neighbors
            CheckVertex(indices.Find(prevIndex));
            CheckVertex(indices.Find(nextIndex));
        }
    }
}
```

### Mesh Generation

```csharp
void MakeMesh()
{
    mesh = new Mesh();
    mesh.vertices = resultVertices.ToArray();
    mesh.SetIndices(resultTriangulation.ToArray(),
                    MeshTopology.Triangles, 0);
    mesh.RecalculateNormals();
    mesh.SetUVs(0, resultUVs);
    mesh.RecalculateBounds();

    GetComponent<MeshFilter>().mesh = mesh;
}
```

---

## Geometry Utilities Summary

| Function | Purpose |
|----------|---------|
| `IsAngleLessPI` | Check if interior angle < 180 degrees |
| `CheckLine` | Determine which side of a line a point is on |
| `IsInTriangle` | Point-in-triangle test |
| `GetIntersectionPoint` | Find line-polygon intersection |
| `IsIntersectLine` | Check if two line segments intersect |

---

## Key Takeaways

1. **Ear Clipping** is conceptually simple but not the fastest triangulation algorithm
2. **Two Ears Theorem** guarantees at least 2 ears exist in any simple polygon
3. **Ear criteria**: Convex vertex + empty triangle interior
4. **Holes** are handled by connecting inner polygons to outer via bridges
5. **Nested polygons** require hierarchy construction with alternating winding orders
6. **LinkedList** provides efficient vertex removal during the algorithm
7. **2D cross product** is key for point-line orientation tests

## Applications

- Real-time mesh generation from user-drawn shapes
- Font outline to mesh conversion
- Procedural level geometry
- Any polygon-to-triangles conversion need

## Performance Note

Ear Clipping has O(n^2) time complexity in the worst case. For performance-critical applications with many vertices, consider faster algorithms like:
- Monotone polygon decomposition + triangulation
- Delaunay triangulation
- Constrained Delaunay triangulation

## References

- Triangulation by Ear Clipping - https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
- Wikipedia: Polygon triangulation
