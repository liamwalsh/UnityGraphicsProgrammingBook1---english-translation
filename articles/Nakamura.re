
= Procedural Modeling with Unity

== Introduction

Procedural Modeling is a technique for building 3D models using rules.
Modeling generally means using the modeling software Blender or 3ds Max to move the vertices and line segments and operate by hand to obtain the desired shape. By contrast, the approach of writing rules and obtaining the shape as a result of a series of automated processes is called procedural modeling.

Procedural modeling has been applied in various fields, and for example, in games, it is used for generation of terrain, modeling of plants, construction of cities, etc. It enables content design such as changing the structure.

Also in the field of architecture and product design, the method of procedurally designing shapes is actively used using Grasshopper@<fn>{grasshopper}, which is a CAD software plug-in called Rhinoceros@<fn>{rhinoceros}. It has been. LW:TODO:edit
//footnote[rhinoceros][http://www.rhino3d.co.jp/]
//footnote[grasshopper][http://www.grasshopper3d.com/]

With procedural modeling, you can:

 * Can create parametric structures
 * A flexible, changeable and dynamic model can be incorporated into the content.

=== Creating Parametric Structures

A parametric structure is a structure in which the elements of the structure can be deformed according to certain parameters. For example, in the case of a sphere model, the radius representing the size and the smoothness of the sphere Parameters such as the number of segments to represent can be defined, and by changing those values, a sphere with the desired size and smoothness can be obtained.

Once you have implemented the program that defines the parametric structure, you can get a model with a specific structure in various situations, which is convenient.

=== A flexible, dynamic model

As mentioned above, in fields such as games, there are many cases where procedural modeling is used to generate terrain and trees. Instead of incorporating what was once exported as a model, it is generated in real time in the content. There are also cases.
By using procedural modeling techniques for real-time content, you can create trees that grow toward the sun at any position, or build a city so that buildings line up from the clicked position. These are just illustrations of the type of content that can be realized.

In addition, if you incorporate various patterns of models into the content, the data size will swell, but you can reduce the data size by using procedural modeling to increase the variation of the model.

By learning procedural modeling techniques and building models programmatically, it will be possible to develop the modeling tools themselves.

== Model representation in Unity

In Unity, the geometry data that represents the shape of the model is managed by the Mesh class.

The shape of the model consists of triangles arranged in 3D space, where one triangle is defined by three vertices.
The official Unity document explains how to manage the vertex and triangle data of the model in the Mesh class as follows.

//quote{
A mesh consists of triangles arranged in 3D space to create the impression of a solid object. A triangle is defined by its three corner points or vertices. In the Mesh class, the vertices are all stored in a single array and each triangle is specified using three integers that correspond to indices of the vertex array. The triangles are also collected together into a single array of integers; the integers are taken in groups of three from the start of this array, so elements 0, 1 and 2 define the first triangle, 3, 4 and 5 define the second, and so on. Any given vertex can be reused in as many triangles as desired but there are reasons why you may not want to do this, as explained below.
@<fn>{mesh}

//}

//footnote[mesh][https://docs.unity3d.com/Manual/AnatomyofaMesh.html]

The model has uv coordinates that represent the coordinates on the texture that are necessary for texture mapping so as to correspond to each vertex, and a normal vector (also called normal) that is needed to calculate the influence of the light source during lighting. Will be included.

==== Sample repository

In this chapter, the following Assets/Procedural Modeling in https://github.com/IndieVisualLab/UnityGraphicsProgramming repository is prepared as a sample program.

Since the model description by C# script is the main explanation content,
We will continue to explain while referring to the C# scripts under Assets/ProceduralModeling/Scripts.

===== Execution environment

The sample code in this chapter has been confirmed to work with Unity 5.0 or higher.

=== Quad

Using the basic model Quad as an example, I will explain how to build a model programmatically.
Quad is a square model that combines two triangles with four vertices, and is provided as a Primitive Mesh by default in Unity, but since it is the most basic shape, it is an example to understand the structure of the model. Useful

//image[ProceduralModeling_quad][Quadモデルの構造　黒丸はモデルの頂点を表し、黒丸内の0〜3の数字は頂点のindexを示している　矢印は一枚の三角形を構築する頂点indexの指定順（右上は0,1,2の順番で指定された三角形、左下は2,3,0の順番で指定された三角形）]{
//}

==== Sample program Quad.cs

First, create an instance of the Mesh class.

//emlist[][cs]{
// Create an instance of Mesh
var mesh = new Mesh();
//}


Next, create a Vector3 array that represents the four vertices at the four corners of the Quad.
Also, prepare uv coordinate and normal data so that they correspond to each of the four vertices.


//emlist[][cs]{
// Quad: Find half the length so that the width and height of are each size
var hsize = size * 0.5f; 

// Quad: Vertex data of
var vertices = new Vector3[] {
    new Vector3(-hsize,  hsize, 0f), // The top left position of the first vertex Quad
    new Vector3( hsize,  hsize, 0f), // Upper right position of second vertex Quad
    new Vector3( hsize, -hsize, 0f), // Lower right position of third vertex Quad
    new Vector3(-hsize, -hsize, 0f)  // Lower left position of fourth vertex Quad
};

// Quad uv coordinate data
var uv = new Vector2[] {
    new Vector2(0f, 0f), // Uv coordinate of the 1st vertex
    new Vector2(1f, 0f), // Uv coordinate of the 2nd vertex
    new Vector2(1f, 1f), // Uv coordinate of the 3rd vertex
    new Vector2(0f, 1f)  // Uv coordinate of the 4th vertex
};

// Quad normal data
var normals = new Vector3[] {
    new Vector3(0f, 0f, -1f), // 1st vertex normal
    new Vector3(0f, 0f, -1f), // 2nd vertex normal
    new Vector3(0f, 0f, -1f), // 3rd vertex normal
    new Vector3(0f, 0f, -1f)  // 4th vertex normal
};
//}


Next, generate triangle data representing the faces of the model. Triangle data is specified by an integer array, and each integer corresponds to the index of the vertex array.


//emlist[][cs]{
// Quad face data, categorize three face vertices as one triangle face
var triangles = new int[] {
    0, 1, 2, // First triangle
    2, 3, 0  // Second triangle
};
//}


Set the last generated data to the Mesh instance.


//emlist[][cs]{
mesh.vertices = vertices;
mesh.uv = uv;
mesh.normals = normals;
mesh.triangles = triangles;

// Calculate the boundary area occupied by Mesh (required for culling)
mesh.RecalculateBounds();

return mesh;
//}

=== ProceduralModelingBase

The sample code used in this chapter uses a base class called ProceduralModelingBase.
In the inherited class of this class, a new Mesh instance is created each time the model parameter (for example, the size representing the width and height in Quad) is changed, and it is applied to the MeshFilter, so that the change result can be immediately confirmed. I can. (This function is realized by using Editor script. ProceduralModelingEditor.cs)

In addition, by changing the enum type parameter called ProceduralModelingMaterial, you can visualize the UV coordinates and normal direction of the model.

//image[ProceduralModeling_materials][From the left, ProceduralModelingMaterial.Standard、ProceduralModelingMaterial.UV、ProceduralModelingMaterial.Normal Applied model]{
//}

== Primitive shape


Now that you understand the model structure, let's create some primitive shapes.


=== Plane


Plane is like quads arranged on a grid.

//image[ProceduralModeling_plane][Plane model]{
//}

Decide the number of rows and columns in the grid, place vertices at the intersections of the grids, construct a Quad so as to fill each grid grid, and combine them to generate one Plane model.

In the sample program Plane.cs, the number of vertical vertices of the Plane heightSegments, the number of vertical vertices widthSegments, and the parameters of the vertical length height and the horizontal length width are prepared.
Each parameter affects the shape of the plane as shown in the figure below.

//image[ProceduralModeling_plane_parameters][Plane parameter]{
//}

==== Sample program Plane.cs

First, create the vertex data to be placed at the intersection points of the grid.

//emlist[][cs]{
var vertices = new List<Vector3>();
var uv = new List<Vector2>();
var normals = new List<Vector3>();

// The reciprocal of the number of matrices to calculate the ratio of vertices 
// on the grid (0.0 to 1.0) 
// LW:TODO (reciprocal?)
var winv = 1f / (widthSegments - 1);
var hinv = 1f / (heightSegments - 1);

for(int y = 0; y < heightSegments; y++) {
    // Percentage of row positions (0.0 to 1.0)
    var ry = y * hinv;

    for(int x = 0; x < widthSegments; x++) {
        // Column position percentage (0.0 to 1.0)
        var rx = x * winv;

        vertices.Add(new Vector3(
            (rx - 0.5f) * width, 
            0f,
            (0.5f - ry) * height
        ));
        uv.Add(new Vector2(rx, ry));
        normals.Add(new Vector3(0f, 1f, 0f));
    }
}
//}

Next, regarding triangle data, the vertex index set for each triangle is referenced as follows in a loop that follows rows and columns.

//emlist[][cs]{
var triangles = new List<int>();

for(int y = 0; y < heightSegments - 1; y++) {
    for(int x = 0; x < widthSegments - 1; x++) {
        int index = y * widthSegments + x;
        var a = index;
        var b = index + 1;
        var c = index + 1 + widthSegments;
        var d = index + widthSegments;

        triangles.Add(a);
        triangles.Add(b);
        triangles.Add(c);

        triangles.Add(c);
        triangles.Add(d);
        triangles.Add(a);
    }
}
//}

==== ParametricPlaneBase

The value of the height (y coordinate) of each vertex of Plane was set to 0, but by operating this height, not only a horizontal surface but also a shape such as uneven terrain and small mountains Can get

The ParametricPlaneBase class inherits the Plane class and overrides the Build function that creates the Mesh.
First, generate the original Plane model, call the Depth(float u, float v) function that finds the height with the uv coordinates of each vertex as input, and set the height flexibly by resetting the height. Transforms.

By implementing a class that inherits this ParametricPlaneBase class, you can generate a Plane model whose height changes depending on the vertices.

==== Sample Program ParametricPlaneBase.cs

//emlist[][cs]{
protected override Mesh Build() {
    // Generate the original Plane model
    var mesh = base.Build();

    // Reset the height of the vertices of the Plane model
    var vertices = mesh.vertices;

    // The reciprocal of the number of matrices to calculate the ratio of vertices on the grid (0.0 to 1.0)
    var winv = 1f / (widthSegments - 1);
    var hinv = 1f / (heightSegments - 1);

    for(int y = 0; y < heightSegments; y++) {
        // Percentage of row position (0.0 ~ 1.0)
        var ry = y * hinv;
        for(int x = 0; x < widthSegments; x++) {
            // Percentage of column positions (0.0-1.0)
            var rx = x * winv;

            int index = y * widthSegments + x;
            vertices[index].y = Depth(rx, ry);
        }
    }

    // reset the vertex position
    mesh.vertices = vertices;
    mesh.RecalculateBounds();

    // Automatically calculate the normal direction
    mesh.RecalculateNormals();

    return mesh;
}
//}


In the sample scene ParametricPlane.scene, the GameObject that uses the class (MountainPlane, TerrainPlane class) that inherits this ParametricPlaneBase is located.
Try changing each parameter and see how the shape changes.

//image[ProceduralModeling_parametric_planes][ParametricPlane.scene　Model generated by MountainPlane class on the left and TerrainPlane class on the right]{
//}

=== Cylinder


Cylinder is a cylindrical model with the shape shown in the following figure.



//image[ProceduralModeling_cylinder][Cylinderの構造]{
//}




The smoothness of a cylindrical circle can be controlled by segments, and the vertical length and thickness can be controlled by the height and radius parameters, respectively.
As shown in the example in the above figure, when 7 is specified for segments, Cylinder becomes a shape in which a regular heptagon is stretched vertically, and it becomes closer to a circle as the value of segments is increased.


==== vertices evenly arranged along the circumference


The Cylinder vertices should be evenly spaced around the circle at the end of the tube.



Use trigonometric functions (Mathf.Sin, Mathf.Cos) to place the vertices evenly distributed along the circumference.
I will omit the details of trigonometric functions here, but by using these functions, the position on the circumference can be obtained based on the angle.



//image[ProceduralModeling_cylinder_trigonometry][Get the position of a point on the circumference from a trigonometric function]{
//}




As shown in this figure, the points located on the circle with radius radius from the angle θ (theta) are obtained by (x, y) = (Mathf.Cos(θ) * radius, Mathf.Sin(θ) * radius) can do.



Based on this, the following process is performed to obtain the positions of segments vertices evenly arranged on the circumference of radius radius.


//emlist[][cs]{
for (int i = 0; i < segments; i++) {
    // 0.0 ~ 1.0
    float ratio = (float)i / (segments - 1);

    // [0.0 ~ 1.0]To[0.0 ~ 2π]Conversion to radians
    float rad = ratio * PI2;

    // Get the position on the circumference
    float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
    float x = cos * radius, y = sin * radius;
}
//}


In Cylinder modeling, vertices are evenly distributed along the circumference of the cylinder at the ends and the vertices are joined together to form the sides.
Just like building a Quad on each side, we take two corresponding vertices from the top and bottom and place the triangles face-to-face to build one side, a rectangle. The side of Cylinder can be imagined as Quads arranged in a circle.



//image[ProceduralModeling_cylinder_sides][Modeling the side of Cylinder Black circles are vertices that are evenly arranged along the circumference of the edge. a to d inside the vertices are index variables that are assigned to the vertices when constructing triangles in the Cylinder.cs program.]{
//}



==== Sample program Cylinder.cs


First of all, we will build the side surface, but in the Cylinder class, we have a function GenerateCap for generating data of vertices arranged on the circumference located at the top and bottom.


//emlist[][cs]{
var vertices = new List<Vector3>();
var normals = new List<Vector3>();
var uvs = new List<Vector2>();
var triangles = new List<int>();

// Top height and bottom height
float top = height * 0.5f, bottom = -height * 0.5f;

// Generates vertex data that constitutes the side surface
GenerateCap(segments + 1, top, bottom, radius, vertices, uvs, normals, true);

// To refer to the vertices on the circle when building the side triangle,
// index is the divisor by which the circle goes around
var len = (segments + 1) * 2;

// Build a side by joining the top and bottom edges
for (int i = 0; i < segments + 1; i++) {
    int idx = i * 2;
    int a = idx, b = idx + 1, c = (idx + 2) % len, d = (idx + 3) % len;
    triangles.Add(a);
    triangles.Add(c);
    triangles.Add(b);

    triangles.Add(d);
    triangles.Add(b);
    triangles.Add(c);
}
//}


Generate CapIn the function, set the vertex and normal data to the variables passed in List type.


//emlist[][cs]{
void GenerateCap(
    int segments, 
    float top, 
    float bottom, 
    float radius, 
    List<Vector3> vertices, 
    List<Vector2> uvs, 
    List<Vector3> normals, 
    bool side
) {
    for (int i = 0; i < segments; i++) {
        // 0.0 ~ 1.0
        float ratio = (float)i / (segments - 1);

        // 0.0 ~ 2π
        float rad = ratio * PI2;

        // Place vertices evenly along the circumference at the top and bottom
        float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
        float x = cos * radius, z = sin * radius;
        Vector3 tp = new Vector3(x, top, z), bp = new Vector3(x, bottom, z);

        // Upper end
        vertices.Add(tp); 
        uvs.Add(new Vector2(ratio, 1f));

        // lower end
        vertices.Add(bp); 
        uvs.Add(new Vector2(ratio, 0f));

        if(side) {
            // Normals that face the outside of the side
            var normal = new Vector3(cos, 0f, sin);
            normals.Add(normal);
            normals.Add(normal);
        } else {
            normals.Add(new Vector3(0f, 1f, 0f)); // Normal facing up the lid
            normals.Add(new Vector3(0f, -1f, 0f)); // Normal facing down the lid
        }
    }
}
//}


In Cylinder class, you can set with openEnded flag whether to make the model with the top and bottom closed. If you want to close the top and bottom, shape a circular "lid" and plug the ends.



The vertices that make up the surface of the lid do not use the vertices that make up the sides, but a new vertex is created at the same position as the sides. This is to separate the normals between the sides and the lid to give natural lighting. (When constructing the side vertex data, true is set to the side variable of the argument of GenerateCap, false is specified when constructing the lid, and the appropriate normal direction is set.)



If the side and the lid share the same vertex, the side and lid will refer to the same normal, which makes the lighting unnatural.


//image[ProceduralModeling_cylinder_lighting][When the side of Cylinder and the top of the lid are shared (left: BadCylinder.cs), and when another vertex is prepared as in the sample program (right: Cylinder.cs), the lighting is unnatural on the left]{
//}




To model a circular lid,（GenerateCap Generated from a function）Prepare vertices evenly arranged on the circumference and vertices located in the middle of the circle, connect the vertices along the circumference from the middle vertices, and build a triangle like an evenly divided pizza To form a circular lid.



//image[ProceduralModeling_cylinder_end][CylinderModeling the lid of the case where the segments parameter is 6]{
//}



//emlist[][cs]{
// // create top and bottom lids
if(openEnded) {
    // New vertices for lid model, not shared with sides, to use different normals for lighting
    GenerateCap(
        segments + 1, 
        top, 
        bottom, 
        radius, 
        vertices, 
        uvs, 
        normals, 
        false
    );

    // The top apex in the middle of the top lid
    vertices.Add(new Vector3(0f, top, 0f));
    uvs.Add(new Vector2(0.5f, 1f));
    normals.Add(new Vector3(0f, 1f, 0f));

    // Middle apex of bottom lid
    vertices.Add(new Vector3(0f, bottom, 0f)); // bottom
    uvs.Add(new Vector2(0.5f, 0f));
    normals.Add(new Vector3(0f, -1f, 0f));

    var it = vertices.Count - 2;
    var ib = vertices.Count - 1;

    // Offset for not referencing the vertex index of the side
    var offset = len;

    // Top lid surface
    for (int i = 0; i < len; i += 2) {
        triangles.Add(it);
        triangles.Add((i + 2) % len + offset);
        triangles.Add(i + offset);
    }

    // Bottom lid surface
    for (int i = 1; i < len; i += 2) {
        triangles.Add(ib);
        triangles.Add(i + offset);
        triangles.Add((i + 2) % len + offset);
    }
}
//}

=== Tubular


Tubular is a tubular model that looks like the following figure.



//image[ProceduralModeling_tubular][Tubular model]{
//}


The Cylinder model has a straight, cylindrical shape, while the Tubular has a cylindrical shape that does not twist along a curve.
In the example of the tree model described below, we use a method of expressing one branch with Tubular and constructing a single tree with that combination, but in the scene where a smoothly bending tubular type is required, Tubular I will play an active role.


==== Cylindrical structure


The structure of the tubular model is as shown in the figure below.



//image[ProceduralModeling_tubular_structure][Cylindrical structure: The points that divide the curve along which the Tubular follows are visualized as spheres, and the nodes that make up the side surface are visualized as hexagons.]{
//}




The curve is divided, the sides are constructed for each node separated by the dividing points, and these are combined to generate one Tubular model.



The side of each knot is the same as the side of Cylinder, because the top and bottom vertices of the side are evenly arranged along the circle and they are connected together to build.
You can think of the Tubular type as connecting Cylinders along a curve.


==== About curves

In the sample program, the base class CurveBase that represents the curve is prepared.
Various algorithms have been devised for how to draw a curve in a three dimensional space, and it is necessary to select an easy-to-use method according to the application.
In the sample program, the class CatmullRomCurve that inherits the CurveBase class is used.



I will omit the details here, but CatmullRomCurve has the feature that it forms a curve while interpolating between points so that it passes through all the control points passed, and it is easy to use because you can specify the point you want to pass through the curve. It has a good reputation for its performance and ease of use.



CurveBase class that represents a curve provides GetPointAt(float) and GetTangentAt(float) functions to obtain the position and slope (tangent vector) of a point on the curve, and specify the value of [0.0 to 1.0] as an argument. By doing so, the position and inclination of the point between the start point (0.0) and the end point (1.0) can be acquired.


==== Frenet frame


In order to create a twist-free cylindrical shape along a curved line, three orthogonal vectors that change smoothly along the curved line: a tangent vector, a normal vector, and a binormal vector. An array is required. The tangent vector is a unit vector that represents the slope at one point on the curve. The normal vector and the binormal vector are obtained as mutually orthogonal vectors.

With these orthogonal vectors, it is possible to obtain "coordinates on the circle orthogonal to the curve" at a certain point on the curve.

//image[ProceduralModeling_tubular_trigonometry][Normal（normal）And the binormal（binormal）From this, find the unit vector (v) that points to the coordinates on the circumference. By multiplying this unit vector (v) by the radius radius, you can obtain the coordinates on the circumference of the radius radius orthogonal to the curve.]{
//}

A set of three orthogonal vectors at one point on this curve is called a Frenet frame.

//image[ProceduralModeling_tubular_frenet_frame][Visualization of Frenet frame array that composes Tubular Represents a frame, the three arrows are tangent Vector, normal Vector, showing binormal vector]{
//}

Tubular modeling is performed by obtaining vertex data for each node based on the normals and binormals obtained from this Frenet frame, and connecting them.

In the sample program, the CurveBase class has a function ComputeFrenetFrames to generate this Frenet frame array.

==== Sample program Tubular.cs


The Tubular class has a CatmullRomCurve class that represents a curve, and forms a cylinder along the curve that this CatmullRomCurve draws.The Tubular class has a CatmullRomCurve class that represents a curve, and forms a cylinder along the curve that this CatmullRomCurve draws.


The CatmullRomCurve class requires four or more control points, and when the control points are manipulated, the shape of the curve changes and the shape of the Tubular model changes accordingly.


//emlist[][cs]{
var vertices = new List<Vector3>();
var normals = new List<Vector3>();
var tangents = new List<Vector4>();
var uvs = new List<Vector2>();
var triangles = new List<int>();

// Get Frenet frame from the curve
var frames = curve.ComputeFrenetFrames(tubularSegments, closed);

// Tubularの頂点データを生成
for(int i = 0; i < tubularSegments; i++) {
    GenerateSegment(curve, frames, vertices, normals, tangents, i);
}
// If you want to create a closed cylinder, place the last vertex at the beginning of the curve, otherwise place it at the end of the curve.
GenerateSegment(
    curve, 
    frames, 
    vertices, 
    normals, 
    tangents, 
    (!closed) ? tubularSegments : 0
);

// Set uv coordinates from the start point to the end point of the curve
for (int i = 0; i <= tubularSegments; i++) {
    for (int j = 0; j <= radialSegments; j++) {
        float u = 1f * j / radialSegments;
        float v = 1f * i / tubularSegments;
        uvs.Add(new Vector2(u, v));
    }
}

// Build side
for (int j = 1; j <= tubularSegments; j++) {
    for (int i = 1; i <= radialSegments; i++) {
        int a = (radialSegments + 1) * (j - 1) + (i - 1);
        int b = (radialSegments + 1) * j + (i - 1);
        int c = (radialSegments + 1) * j + i;
        int d = (radialSegments + 1) * (j - 1) + i;

        triangles.Add(a); triangles.Add(d); triangles.Add(b);
        triangles.Add(b); triangles.Add(d); triangles.Add(c);
    }
}

var mesh = new Mesh();
mesh.vertices = vertices.ToArray();
mesh.normals = normals.ToArray();
mesh.tangents = tangents.ToArray();
mesh.uv = uvs.ToArray();
mesh.triangles = triangles.ToArray();
//}


The function GenerateSegment calculates the vertex data of the specified node based on the normals and binormals extracted from the Frenet frame described above and sets it in the variable passed in List type.


//emlist[][cs]{
void GenerateSegment(
    CurveBase curve, 
    List<FrenetFrame> frames, 
    List<Vector3> vertices, 
    List<Vector3> normals, 
    List<Vector4> tangents, 
    int index
) {
    // 0.0 ~ 1.0
    var u = 1f * index / tubularSegments;

    var p = curve.GetPointAt(u);
    var fr = frames[index];

    var N = fr.Normal;
    var B = fr.Binormal;

    for(int j = 0; j <= radialSegments; j++) {
        // 0.0 ~ 2π
        float rad = 1f * j / radialSegments * PI2;

        // Distribute vertices evenly along the circumference
        float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
        var v = (cos * N + sin * B).normalized;
        vertices.Add(p + radius * v);
        normals.Add(v);

        var tangent = fr.Tangent;
        tangents.Add(new Vector4(tangent.x, tangent.y, tangent.z, 0f));
    }
}
//}

== Complex shape


This section introduces techniques for generating more complex models using the procedural modeling techniques described so far.


=== plant


Plant modeling is often cited as an application of Procedural Modeling techniques.
Tree API @<fn>{tree} is available for modeling trees in Editor in Unity.
There is software dedicated to plant modeling called Speed Tree@<fn>{speedtree}.

//footnote[tree][https://docs.unity3d.com/ja/540/Manual/tree-FirstTree.html]
//footnote[speedtree][http://www.speedtree.com/]


This section deals with modeling trees, which are relatively simple modeling methods among plants.


=== L-System


L-System is an algorithm that can describe and express the structure of plants.
The L-System was advocated by the botanist Aristid Lindenmayer in 1968, and the L for the L-System comes from his name.



L-System can be used to express the self-similarity found in plant shapes.



Self-similarity means that when the shape of a detail of an object is enlarged, it matches the shape of the object seen on a large scale.
For example, when observing the branching of trees, there is similarity in the way of branching near the trunk and the way of branching near the tip.

//image[ProceduralModeling_tree_lsystem][Figure that each branch is branched by 30 degrees change It can be seen that the root part and the branch part are similar, but even such a simple figure looks like a tree (sample program LSystem .scene)]{
//}




The L-System provides a mechanism to develop a complex sequence of symbols by expressing elements by symbols, defining rules for replacing symbols, and applying the rules repeatedly to symbols.



A simple example

 * Initial character string: a



To

 * Rewrite rule 1: a -> ab
 * Rewrite rule 2: b -> a



When rewriting according to



a -> ab -> aba -> abaab -> abaababa -> ...



It produces complicated results with each step.



An example of using this L-System for graphic generation is the LSystem class of the sample program.



In LSystem class, the following operations

 * Draw: Draw a line in the direction you are facing
 * Turn Left: Rotate θ degrees to the left
 * Turn Right: Turn θ degrees to the right



Are available,

 * Initial operation: Draw

To

 * Rewrite Rule 1: Draw -> Turn Left | Turn Right
 * Rewrite Rule 2: Turn Left -> Draw
 * Rewrite Rule 3: Turn Right -> Draw

According to, the rules are applied a fixed number of times.


As a result, you can draw a diagram with self-similarity, as shown in the sample LSystem.scene.
The property of "recursively rewriting state" of this L-System creates self-similarity.
Self-similarity, also called Fractal, is a research area.


=== Sample program ProceduralTree.cs


As an example of actually applying L-System to a program for generating a tree model, we have prepared a class called ProceduralTree.



In ProceduralTree, the tree shape is generated by recursively calling the routine "advance a branch, then a branch, and then an additional branch" as in the LSystem class described in the previous section.


In the LSystem class of the previous section, regarding the branching of the branch, it was a simple rule to "branch in two directions, left and right, at a fixed angle",
Procedural Tree uses random numbers to give randomness to the number of branches and the direction of branching, and sets rules such that branches branch in a complicated manner.



//image[ProceduralModeling_tree_ProceduralTree][ProceduralTree.scene]{
//}



==== TreeData class


TreeData class is a class that includes parameters that determine the branching condition of branches, parameters that determine the size of the tree and the fineness of the model mesh.
You can design a tree shape by adjusting the parameters of the instance of this class.


==== Branching


Adjust the branching using some parameters in the TreeData class.


===== branchesMin, branchesMax


The number of branches that branch from one branch is adjusted by the branchesMin/branchesMax parameter.
branchesMin represents the minimum number of branches and branchesMax represents the maximum number of branches. The number between branchesMin and branchesMax is randomly selected to determine the number of branches.


===== growthAngleMin, growthAngleMax, growthAngleScale

The direction in which the branching branches grow is adjusted with the growthAngleMin and growthAngleMax parameters.
growthAngleMin represents the minimum angle of the branching direction, growthAngleMax represents the maximum angle, and the number between growthAngleMin and growthAngleMax is randomly selected to determine the branching direction.



Each branch has a tangent vector that represents the direction in which it extends, and a normal vector and a binormal vector that are orthogonal to it.



The values randomly obtained from the growthAngleMin and growAngleMax parameters are rotated in the normal vector direction and the binormal vector direction with respect to the tangent vector extending from the branch point.



By adding a random rotation to the direction tangent vector extending from the branch point, the direction in which the branch destination branch grows is changed, and the branching is complicatedly changed.

//image[ProceduralModeling_tree_branches][Random rotation applied to the direction extending from the branch point T arrow at the branch point is the extending direction (tangent vector), N arrow is the normal line (normal vector), B arrow is the binormal line (binormal vector) Represents a random rotation in the normal and binormal directions with respect to the extending direction.]{
//}


The growthAngleScale parameter is prepared so that the angle of rotation that is randomly applied in the direction in which the branch grows becomes larger as it goes to the tip of the branch.
This growthAngleScale parameter strongly influences the angle of rotation as the generation parameter representing the generation of the branch instance approaches 0, that is, the edge of the branch, and increases the angle of rotation.


//emlist[][cs]{
// The branching angle increases as the end of the branch branches
var scale = Mathf.Lerp(
    1f, 
    data.growthAngleScale, 
    1f - 1f * generation / generations
);

// rotation in the normal direction
var qn = Quaternion.AngleAxis(scale * data.GetRandomGrowthAngle(), normal);

// rotation in binormal direction
var qb = Quaternion.AngleAxis(scale * data.GetRandomGrowthAngle(), binormal);

// Determine the position of the branch tip by applying qn * qb rotation in the tangent direction in which the branch tip is facing.
this.to = from + (qn * qb) * tangent * length;
//}

==== TreeBranch class

Branches are represented by the TreeBranch class.

In addition to the parameters of the number of generations (generations) and the basic length (length) and thickness (radius), when you call the constructor by specifying the TreeData for setting the branching pattern as an argument, recursively inside TreeBranch instances will be created.

The TreeBranch branched from one TreeBranch is stored in the child variable of List<TreeBranch> type in the original TreeBranch, and all branches can be traced from the root TreeBranch.

==== TreeSegment class

Similar to Tubular, the model of one branch is constructed by dividing one curve, modeling the divided nodes as one Cylinder, and connecting them together.

TreeSegment class is a class that represents a segment (Segment) that divides one curve.


//emlist[][cs]{
public class TreeSegment {
    public FrenetFrame Frame { get { return frame; } }
    public Vector3 Position { get { return position; } }
    public float Radius { get { return radius; } }

    // Direction vector tangent that TreeSegment is facing,
    // FrenetFrame with vectors normal and binormal orthogonal to it
    FrenetFrame frame;

    // TreeSegmentの位置
    Vector3 position;

    // TreeSegmentの幅(半径)
    float radius;

    public TreeSegment(FrenetFrame frame, Vector3 position, float radius) {
        this.frame = frame;
        this.position = position;
        this.radius = radius;
    }
}
//}


One TreeSegment has FrenetFrame which is a set of vector of the direction in which the node is facing and orthogonal vector, variables that represent position and width, and holds the information required at the top and bottom when building the Cylinder.


==== Procedural Tree model generation


The model generation logic of ProceduralTree is an application of Tubular. It generates a Tubular model from the TreeSegment array of one branch TreeBranch.
Modeling is done by aggregating them into a single model to form an entire tree.


//emlist[][cs]{
var root = new TreeBranch(
    generations, 
    length, 
    radius, 
    data
);

var vertices = new List<Vector3>();
var normals = new List<Vector3>();
var tangents = new List<Vector4>();
var uvs = new List<Vector2>();
var triangles = new List<int>();

// Get the full length of a tree
// By dividing the length of the branch by the total length, the height of the uv coordinate (uv.y) is
// Set to change from [0.0 to 1.0] from root to branch
float maxLength = TraverseMaxLength(root);

//Recursively traverses all branches and creates a mesh corresponding to each branch
Traverse(root, (branch) => {
    var offset = vertices.Count;

    var vOffset = branch.Offset / maxLength;
    var vLength = branch.Length / maxLength;

    // Generate vertex data from one branch
    for(int i = 0, n = branch.Segments.Count; i < n; i++) {
        var t = 1f * i / (n - 1);
        var v = vOffset + vLength * t;

        var segment = branch.Segments[i];
        var N = segment.Frame.Normal;
        var B = segment.Frame.Binormal;
        for(int j = 0; j <= data.radialSegments; j++) {
            // 0.0 ~ 2π
            var u = 1f * j / data.radialSegments;
            float rad = u * PI2;

            float cos = Mathf.Cos(rad), sin = Mathf.Sin(rad);
            var normal = (cos * N + sin * B).normalized;
            vertices.Add(segment.Position + segment.Radius * normal);
            normals.Add(normal);

            var tangent = segment.Frame.Tangent;
            tangents.Add(new Vector4(tangent.x, tangent.y, tangent.z, 0f));

            uvs.Add(new Vector2(u, v));
        }
    }

    // Construct a triangle with one branch
    for (int j = 1; j <= data.heightSegments; j++) {
        for (int i = 1; i <= data.radialSegments; i++) {
            int a = (data.radialSegments + 1) * (j - 1) + (i - 1);
            int b = (data.radialSegments + 1) * j + (i - 1);
            int c = (data.radialSegments + 1) * j + i;
            int d = (data.radialSegments + 1) * (j - 1) + i;

            a += offset;
            b += offset;
            c += offset;
            d += offset;

            triangles.Add(a); triangles.Add(d); triangles.Add(b);
            triangles.Add(b); triangles.Add(d); triangles.Add(c);
        }
    }
});

var mesh = new Mesh();
mesh.vertices = vertices.ToArray();
mesh.normals = normals.ToArray();
mesh.tangents = tangents.ToArray();
mesh.uv = uvs.ToArray();
mesh.triangles = triangles.ToArray();
mesh.RecalculateBounds();
//}


For procedural modeling of plants, methods have been devised such as deep in trees alone, and by branching so that the irradiation rate of sunlight is high, a natural tree model is obtained.

For those who are interested in modeling such plants, various methods are introduced in The Algorithmic Beauty of Plants@<fn>{abop} written by Aristid Lindenmayer who invented L-System, so please refer to it. ..

//footnote[abop][http://algorithmicbotany.org/papers/#abop]


== Application example of procedural modeling


From the examples of procedural modeling introduced so far, you can see the advantage of the technique that "models can be dynamically generated while changing with parameters".
Since you can efficiently create various variations of models, you may get the impression that it is a technology for streamlining content development.


However, like the modeling tools and sculpting tools in the world, the technique of procedural modeling can be applied to "generate a model interactively according to the user's input".


As an application example, we would like to introduce "Teddy," a technology that was invented by Takeo Igarashi of the University of Tokyo's Graduate School of Information Engineering to generate a three-dimensional model from contour lines created by handwriting sketches.


//image[ProceduralModeling_teddy][Unity asset of "Teddy" technology for 3D modeling by handwriting sketch　http://uniteddy.info/ja]{
//}


This technology was actually used in a game called "Garagaku Masterpiece Theater: Rakugaki Kingdom" @<fn>{rakugaki}, which was released as software for PlayStation 2 in 2002. The application of "moving as a character" has been realized.

//footnote[rakugaki][https://ja.wikipedia.org/wiki/ラクガキ王国]

With this technology,

  * Define a line drawn on a 2D plane as a contour
  * Perform a meshing process called Delaunay Triangulation @<fn>{delaunay} on the array of points that make up the contour line
  * Apply an algorithm to inflate the mesh on the obtained 2D plane

//footnote[delaunay][https://en.wikipedia.org/wiki/Delaunay_triangulation]

The 3D model is generated by the procedure.
For details of the algorithm, a paper presented at SIGGRAPH, an international conference dealing with computer graphics, has been published. @<fn>{teddy}

//footnote[teddy][http://www-ui.is.s.u-tokyo.ac.jp/~takeo/papers/siggraph99.pdf]


A version of Teddy that has been ported to Unity is available on the Asset Store, so anyone can incorporate this technology into their content.
@<fn>{uniteddy}

//footnote[uniteddy][http://uniteddy.info]


By using procedural modeling techniques like this, you can develop your own modeling tool,
It is also possible to create content that will evolve as the user creates it.


== Summary


With procedural modeling techniques,

 * Efficient model generation (under certain conditions)
 * Development of tools and contents that interactively generate models according to user operations

We have seen that can be realized.

Unity itself is a game engine, so you can imagine its application in games and video content from the examples presented in this chapter.

However, just as the computer graphics technology itself has a wide range of applications, it can be considered that the technology of model generation has a wide range of applications.
As I mentioned at the beginning, procedural modeling methods are used in the fields of architecture and product design,
With the development of digital fabrication such as 3D printer technology, opportunities to use designed shapes in real life are increasing at the individual level.

In this way, considering in which field the designed shape is used,
There may be many places where you can apply procedural modeling techniques.

== reference
 * Computers intelligently generate content--what is procedural technology?
 * The Algorithmic Beauty of Plants - http://algorithmicbotany.org/papers
 * nervous system - http://n-e-r-v-o-u-s.com/

