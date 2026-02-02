# Chapter 7: Shape Matching - Applying Linear Algebra to CG

**Author**: Takao

**Sample Project**: "ShapeMatching" in the [Unity Graphics Programming 2 Repository](https://github.com/IndieVisualLab/UnityGraphicsProgramming2)

---

## Introduction

This chapter covers the fundamentals and applications of linear algebra, leading to Singular Value Decomposition (SVD) and its application in Shape Matching. Many people learn matrices in high school and linear algebra in university but struggle to see how these concepts apply in CG - this chapter bridges that gap.

For accessibility, explanations are limited to **2D with real numbers**. While some definitions differ slightly from formal linear algebra, the core concepts remain valid.

---

## Part 1: Matrix Fundamentals Review

### What is a Matrix?

A matrix is numbers arranged in rows and columns:

$$M = \begin{pmatrix} 1 & 2 \\ 3 & 4 \end{pmatrix}$$

**Terminology**:
- **Row** - horizontal direction
- **Column** - vertical direction
- **Diagonal** - from top-left to bottom-right
- **Elements** - individual numbers
- **Matrix** - "Matrix" in English

### Basic Operations

For 2x2 matrices **A** and **B**, and vector **c**:

**Addition** - element-wise:
$$A + B = \begin{pmatrix} a_{00}+b_{00} & a_{01}+b_{01} \\ a_{10}+b_{10} & a_{11}+b_{11} \end{pmatrix}$$

**Subtraction** - element-wise:
$$A - B = \begin{pmatrix} a_{00}-b_{00} & a_{01}-b_{01} \\ a_{10}-b_{10} & a_{11}-b_{11} \end{pmatrix}$$

**Multiplication** - more complex, order matters:
$$AB \neq BA$$ (in general)

**Inverse Matrix** - the "division" equivalent:
- For scalars: $4 \times \frac{1}{4} = 1$
- For matrices: $M \cdot M^{-1} = M^{-1} \cdot M = I$

The **identity matrix** $I$ is the matrix equivalent of 1:
$$I = \begin{pmatrix} 1 & 0 \\ 0 & 1 \end{pmatrix}$$

**Inverse formula for 2x2**:
$$A^{-1} = \frac{1}{a_{00}a_{11} - a_{01}a_{10}} \begin{pmatrix} a_{11} & -a_{01} \\ -a_{10} & a_{00} \end{pmatrix}$$

The denominator $(a_{00}a_{11} - a_{01}a_{10})$ is the **determinant**, written $\det(A)$.

### Matrix-Vector Multiplication (Transformation)

Matrices transform vectors - the foundation of coordinate transformations in CG:
$$Ac = \begin{pmatrix} a_{00}c_0 + a_{01}c_1 \\ a_{10}c_0 + a_{11}c_1 \end{pmatrix}$$

---

## Part 2: Advanced Matrix Concepts

### Transpose

Swap rows and columns:
$$A^T = \begin{pmatrix} a_{00} & a_{10} \\ a_{01} & a_{11} \end{pmatrix}$$

### Symmetric Matrix

A matrix where $A^T = A$.

### Eigenvalues and Eigenvectors

For a square matrix **A**, if:
$$A\vec{v} = \lambda\vec{v}$$ (where $\vec{v} \neq 0$)

Then $\lambda$ is an **eigenvalue** and $\vec{v}$ is an **eigenvector**.

**Intuition**: Eigenvectors are special directions where the matrix only scales (by $\lambda$), without rotation.

**Computing eigenvalues**: Solve $\det(A - \lambda I) = 0$, which yields a polynomial equation.

### Eigenvalue Decomposition

A matrix can be decomposed as:
$$A = V \Lambda V^{-1}$$

Where:
- $\Lambda$ = diagonal matrix of eigenvalues (sorted)
- $V$ = columns are corresponding eigenvectors

### Orthonormal Basis

Vectors that are:
- Mutually perpendicular
- All unit length (magnitude = 1)

Example: standard x and y axes: $(1,0)$ and $(0,1)$

### Orthogonal Matrix

A matrix **Q** where columns form an orthonormal set:
$$Q^T Q = I, \quad Q^{-1} = Q^T$$

---

## Part 3: Singular Value Decomposition (SVD)

Any m x n matrix **A** can be decomposed as:
$$A = U \Sigma V^T$$

Where:
- **U** = m x m orthogonal matrix
- **$\Sigma$** = m x n diagonal matrix (non-negative values, sorted)
- **V^T** = n x n orthogonal matrix

### Why SVD Matters

Unlike eigenvalue decomposition (only for square matrices), SVD works on **any** matrix.

### Key Property

For a symmetric matrix, eigenvalues and singular values are identical.

### Computing SVD

Multiply both sides by $A^T$:
$$A^T A = V \Sigma^T \Sigma V^T = V \Sigma^2 V^T$$

This matches eigenvalue decomposition form! So:
1. Compute eigenvalues of $A^T A$
2. Square roots of eigenvalues = singular values
3. Eigenvectors of $A^T A$ = columns of V
4. Compute U from the relationship

### C# Implementation

```csharp
public void SVD(ref Matrix2x2 u, ref Matrix2x2 s, ref Matrix2x2 v)
{
    // Special case: diagonal matrix
    if (Mathf.Abs(this[1, 0] - this[0, 1]) < MATRIX_EPSILON
        && Mathf.Abs(this[1, 0]) < MATRIX_EPSILON)
    {
        u.SetValue(this[0, 0] < 0 ? -1 : 1, 0,
                   0, this[1, 1] < 0 ? -1 : 1);
        s.SetValue(Mathf.Abs(this[0, 0]), Mathf.Abs(this[1, 1]));
        v.LoadIdentity();
    }
    else
    {
        // Compute A^T * A terms
        float i = this[0, 0] * this[0, 0] + this[1, 0] * this[1, 0];
        float j = this[0, 1] * this[0, 1] + this[1, 1] * this[1, 1];
        float i_dot_j = this[0, 0] * this[0, 1] + this[1, 0] * this[1, 1];

        // Orthogonal case
        if (Mathf.Abs(i_dot_j) < MATRIX_EPSILON)
        {
            float s1 = Mathf.Sqrt(i);
            float s2 = Mathf.Abs(i - j) < MATRIX_EPSILON ? s1 : Mathf.Sqrt(j);
            u.SetValue(this[0, 0] / s1, this[0, 1] / s2,
                       this[1, 0] / s1, this[1, 1] / s2);
            s.SetValue(s1, s2);
            v.LoadIdentity();
        }
        // General case: solve quadratic for eigenvalues
        else
        {
            float i_minus_j = i - j;
            float i_plus_j = i + j;
            float root = Mathf.Sqrt(i_minus_j * i_minus_j + 4 * i_dot_j * i_dot_j);
            float eig = (i_plus_j + root) * 0.5f;
            float s1 = Mathf.Sqrt(eig);
            float s2 = Mathf.Abs(root) < MATRIX_EPSILON ? s1 :
                       Mathf.Sqrt((i_plus_j - root) / 2);

            s.SetValue(s1, s2);

            // V from eigenvectors
            float v_s = eig - i;
            float len = Mathf.Sqrt(v_s * v_s + i_dot_j * i_dot_j);
            i_dot_j /= len;
            v_s /= len;
            v.SetValue(i_dot_j, -v_s, v_s, i_dot_j);

            // U = A * V * S^-1
            u.SetValue(
                (this[0, 0] * i_dot_j + this[0, 1] * v_s) / s1,
                (this[0, 1] * i_dot_j - this[0, 0] * v_s) / s2,
                (this[1, 0] * i_dot_j + this[1, 1] * v_s) / s1,
                (this[1, 1] * i_dot_j - this[1, 0] * v_s) / s2
            );
        }
    }
}
```

---

## Part 4: Shape Matching

### Overview

Shape Matching aligns two different shapes with minimal error. It's used in:
- Soft body physics simulation
- Point cloud registration
- Motion capture fitting

### The Problem

Given two point sets P (target) and Q (source) with corresponding points, find rotation **R** and translation **t** to align Q onto P.

### Algorithm

1. **Compute centroids** of both point sets:
   $$\vec{p} = \frac{1}{n}\sum_{i=1}^{n}\vec{p}_i, \quad \vec{q} = \frac{1}{n}\sum_{i=1}^{n}\vec{q}_i$$

2. **Center both sets** (subtract centroids):
   $$\vec{p}'_i = \vec{p}_i - \vec{p}, \quad \vec{q}'_i = \vec{q}_i - \vec{q}$$

3. **Compute covariance matrix H**:
   $$H = \sum_{i=1}^{n} \vec{q}'_i (\vec{p}'_i)^\top$$

   Note: This is the **outer product** (creates a matrix from two vectors)

4. **SVD decomposition** of H:
   $$H = U \Sigma V^T$$

5. **Extract rotation**:
   $$R = V U^T$$

6. **Compute translation**:
   $$\vec{t} = \vec{p} - R\vec{q}$$

### Implementation

```csharp
void Start()
{
    // Initialize point sets and centroids
    p = new Vector2[n];
    q = new Vector2[n];
    centerP = Vector2.zero;
    centerQ = Vector2.zero;

    // Gather points and compute centroids
    for(int i = 0; i < n; i++)
    {
        p[i] = _destination.transform.GetChild(i).position;
        centerP += p[i];
        q[i] = _target.transform.GetChild(i).position;
        centerQ += q[i];
    }
    centerP /= n;
    centerQ /= n;

    // Center points and build covariance matrix
    Matrix2x2 H = new Matrix2x2(0, 0, 0, 0);
    for (int i = 0; i < n; i++)
    {
        p[i] = p[i] - centerP;
        q[i] = q[i] - centerQ;
        H += Matrix2x2.OuterProduct(q[i], p[i]);
    }

    // SVD
    Matrix2x2 u = new Matrix2x2();
    Matrix2x2 s = new Matrix2x2();
    Matrix2x2 v = new Matrix2x2();
    H.SVD(ref u, ref s, ref v);

    // Extract rotation and translation
    R = v * u.Transpose();
    t = centerP - R * centerQ;
}
```

---

## Key Takeaways

| Concept | Application |
|---------|-------------|
| SVD | Decomposes any matrix into rotation-scale-rotation |
| Covariance Matrix | Captures relationship between point sets |
| Shape Matching | Finds optimal rigid transformation between shapes |
| Outer Product | Builds matrices from vector pairs |

---

## Applications of SVD in CG

1. **Shape Matching** - Soft body simulation, registration
2. **Anisotropic Kernels** - Fluid surface reconstruction
3. **Material Point Method** - Snow/material simulation
4. **Principal Component Analysis** - Dimensionality reduction
5. **Image Compression** - Low-rank approximations

---

## Summary

SVD provides a powerful tool for extracting rotation and scale from transformation data. Shape Matching demonstrates how linear algebra concepts translate into practical CG algorithms.

While this chapter covered 2D, the same algorithms extend directly to 3D - only the matrix sizes change.

---

## References

- "3D Geometry for Computer Graphics" (ETH Zurich)
- MIT OpenCourseWare: "The Singular Value Decomposition (SVD)"
- AMATH 301: "The Singular Value Decomposition (SVD)" lecture
- Qiita: "Visualizing Eigenvalues and Eigenvectors" by @kenmatsu4
