# Chapter 8: Space Filling - Apollonian Gaskets

**Author**: Aoyama

**Sample Project**: "SpaceFilling" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter focuses on **Space Filling problems** and explores one solution: the **Apollonian Gasket**. While this chapter diverges slightly from typical graphics programming, the underlying algorithms have fascinating visual applications.

---

## What is the Space Filling Problem?

The Space Filling problem asks: how do you fill a closed region with shapes as completely as possible, without overlapping?

This problem has been studied extensively in geometry and combinatorial optimization. Different shape combinations require different approaches:

| Problem Type | Method |
|--------------|--------|
| Rectangle Packing | O-Tree Method |
| Polygon Packing | Bottom-Left Method |
| Circle Packing | Apollonian Gasket |
| Triangle Packing | Sierpinski Gasket |

**Important Note**: Space filling is NP-hard, meaning no algorithm guarantees 100% coverage. The Apollonian Gasket is no exception - it cannot completely fill a circular area with circles.

---

## The Apollonian Gasket

### Historical Background

The Apollonian Gasket is a type of fractal figure generated from three mutually tangent circles. Named after the ancient Greek mathematician Apollonius of Perga, it emerged from his work in planar geometry rather than as a space-filling algorithm.

### The Core Concept

Given three mutually tangent circles C1, C2, C3, Apollonius discovered that exactly two non-intersecting circles (C4, C5) exist that are tangent to all three.

From here, consider (C1, C2, C4) - this trio has two tangent circles: C3 (already known) and a new circle C6. Repeat this process for all combinations:
- (C1, C2, C5)
- (C2, C3, C4)
- (C1, C3, C4)
- etc.

Repeating infinitely produces the Apollonian Gasket - an infinite set of mutually tangent circles.

### Key Terminology

**Apollonius Circle**: Historically, the locus of points P where AP:BP = constant. In this context, it refers to solutions to Apollonius's Problem.

**Apollonius's Problem**: Given three circles, find a fourth circle tangent to all three. Up to 8 solutions exist, with 2 always externally tangent and 2 always internally tangent.

---

## Implementation

### Prerequisites: Helper Classes

**Circle Class**:
```csharp
public class Circle
{
    public float Curvature => 1f / this.radius;
    public Complex Complex { get; private set; }
    public float Radius => Mathf.Abs(this.radius);
    public Vector2 Position => this.Complex.Vec2;

    private float radius = 0f;

    public Circle(Complex complex, float radius)
    {
        this.radius = radius;
        this.Complex = complex;
    }

    // Methods for checking relationships:
    // IsCircumscribed, IsInscribed, etc.
}
```

**Complex Number Struct**: Required for the complex form of Descartes' Circle Theorem (C# System.Numerics.Complex requires .NET 4.0+, so a custom implementation is provided).

---

### Step 1: Creating Initial Three Circles

The algorithm requires three mutually tangent circles as input. These are generated randomly:

```csharp
private void CreateFirstCircles(out Circle c1, out Circle c2, out Circle c3)
{
    var r1 = Random.Range(firstRadiusMin, firstRadiusMax);
    var r2 = Random.Range(firstRadiusMin, firstRadiusMax);
    var r3 = Random.Range(firstRadiusMin, firstRadiusMax);

    // First circle: random position
    var p1 = GetRandPosInCircle(fieldRadiusMin, fieldRadiusMax);
    c1 = new Circle(new Complex(p1), r1);

    // Second circle: tangent to first
    var p2 = -p1.normalized * ((r1 - p1.magnitude) + r2);
    c2 = new Circle(new Complex(p2), r2);

    // Third circle: tangent to both (using law of cosines)
    var p3 = GetThirdVertex(p1, p2, r1 + r2, r2 + r3, r1 + r3);
    c3 = new Circle(new Complex(p3), r3);
}
```

**Law of Cosines Application**: The three circle centers form a triangle where each side equals the sum of two radii. Using:

$$\cos\alpha = \frac{c^2 + b^2 - a^2}{2cb}$$

We can find the angle, then compute the third center's position.

---

### Step 2: Descartes' Circle Theorem (Computing Radius)

For four mutually tangent circles with curvatures k1, k2, k3, k4:

$$(k_1 + k_2 + k_3 + k_4)^2 = 2(k_1^2 + k_2^2 + k_3^2 + k_4^2)$$

Where **curvature** k = 1/radius.

Solving for k4:

$$k_4 = k_1 + k_2 + k_3 \pm 2\sqrt{k_1 k_2 + k_2 k_3 + k_3 k_1}$$

**Two Solutions**:
- Positive curvature: circle externally tangent to all three
- Negative curvature: circle internally tangent (contains all three)

```csharp
var k1 = Circle1.Curvature;
var k2 = Circle2.Curvature;
var k3 = Circle3.Curvature;

var plusK = k1 + k2 + k3 + 2f * Mathf.Sqrt(k1 * k2 + k2 * k3 + k3 * k1);
var minusK = k1 + k2 + k3 - 2f * Mathf.Sqrt(k1 * k2 + k2 * k3 + k3 * k1);
```

**Note**: These four tangent circles are called **Soddy Circles** (after chemist Frederick Soddy who rediscovered the theorem).

---

### Step 3: Descartes' Complex Circle Theorem (Computing Center)

For centers as complex numbers z1, z2, z3, z4:

$$(k_1 z_1 + k_2 z_2 + k_3 z_3 + k_4 z_4)^2 = 2(k_1^2 z_1^2 + k_2^2 z_2^2 + k_3^2 z_3^2 + k_4^2 z_4^2)$$

Solving for z4:

$$z_4 = \frac{z_1 k_1 + z_2 k_2 + z_3 k_3 \pm 2\sqrt{k_1 k_2 z_1 z_2 + k_2 k_3 z_2 z_3 + k_3 k_1 z_3 z_1}}{k_4}$$

```csharp
var ck1 = Complex.Multiply(Circle1.Complex, k1);
var ck2 = Complex.Multiply(Circle2.Complex, k2);
var ck3 = Complex.Multiply(Circle3.Complex, k3);

var plusZ = ck1 + ck2 + ck3
    + Complex.Multiply(Complex.Sqrt(ck1 * ck2 + ck2 * ck3 + ck3 * ck1), 2f);
var minusZ = ck1 + ck2 + ck3
    - Complex.Multiply(Complex.Sqrt(ck1 * ck2 + ck2 * ck3 + ck3 * ck1), 2f);
```

**Validation**: Unlike radius, one of the two center solutions is correct. Test each candidate:

```csharp
(c1.IsCircumscribed(c4, accuracy) || c1.IsInscribed(c4, accuracy)) &&
(c2.IsCircumscribed(c4, accuracy) || c2.IsInscribed(c4, accuracy)) &&
(c3.IsCircumscribed(c4, accuracy) || c3.IsInscribed(c4, accuracy))
```

---

### Step 4: Recursive Generation

```csharp
private void Awake()
{
    // Create initial three circles
    Circle c1, c2, c3;
    CreateFirstCircles(out c1, out c2, out c3);
    circles.Add(c1);
    circles.Add(c2);
    circles.Add(c3);

    soddys.Enqueue(new SoddyCircles(c1, c2, c3));

    while(soddys.Count > 0)
    {
        var soddy = soddys.Dequeue();
        Circle c4, c5;
        soddy.GetApollonianGaskets(out c4, out c5);
        AddCircle(c4, soddy);
        AddCircle(c5, soddy);
    }
}

private void AddCircle(Circle c, SoddyCircles soddy)
{
    if(c == null || c.Radius <= MinimumRadius) return;

    // Negative curvature circles appear only once
    if(c.Curvature < 0f)
    {
        circles.Add(c);
        soddy.GetSoddyCircles(c).ForEach(s => soddys.Enqueue(s));
        return;
    }

    // Check for overlaps with existing circles
    for(var i = 0; i < circles.Count; i++)
    {
        var o = circles[i];
        if(o.Curvature < 0f) continue;
        if(o.IsMatch(c, CalculationAccuracy)) return;
    }

    circles.Add(c);
    soddy.GetSoddyCircles(c).ForEach(s => soddys.Enqueue(s));
}
```

**Termination Condition**: Since infinite recursion is impossible, stop when new circles become smaller than `MinimumRadius`.

**Overlap Handling**: New circles might overlap with existing ones even if geometrically valid - these must be filtered out.

---

## Key Takeaways

| Concept | Description |
|---------|-------------|
| Curvature | 1/radius - enables elegant mathematical formulations |
| Descartes' Theorem | Relates curvatures of four mutually tangent circles |
| Complex Plane | Natural representation for circle centers in 2D |
| Soddy Circles | Four mutually tangent circles satisfying Descartes' theorem |
| Fractal Nature | Infinite recursion produces self-similar patterns |

---

## Extensions and Applications

### 3D Space Filling

Moving from circles to spheres dramatically increases complexity. The famous **Kepler Conjecture** about sphere packing took centuries to prove mathematically.

### Practical Applications

- **VLSI Layout** - Chip design optimization
- **Material Cutting** - Minimizing waste in fabric/material cutting
- **UV Unwrapping** - Automatic optimization of texture space
- **Procedural Art** - Generative visual patterns

---

## Summary

The Apollonian Gasket demonstrates how classical geometry can address modern computational problems. While historically a fractal curiosity, the underlying algorithms for fitting circles within circles have practical applications in layout optimization and generative art.

The concept of filling space with shapes opens creative possibilities for visual expression in unexpected ways.

---

## References

- [Wikipedia: Apollonian Gasket](https://en.wikipedia.org/wiki/Apollonian_gasket)
- [Wikipedia: Descartes' Circle Theorem](https://en.wikipedia.org/wiki/Descartes%27_theorem)
- [Paul Bourke: Random Tiling](http://paulbourke.net/fractals/randomtile/)
