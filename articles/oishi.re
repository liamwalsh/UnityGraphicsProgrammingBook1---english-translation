
= GPU implementation of flocking / swarm simulation

== Introduction


In this chapter, we will explain the implementation of group simulation using the Boids algorithm with ComputeShader.
Birds, fish and other terrestrial animals sometimes swarm. The movement of this group has regularity and complexity, and it has a certain beauty and has attracted people.
In computer graphics, it is not realistic to control the behavior of each individual one by one, and an algorithm for creating groups called Boids was devised. This simulation algorithm is composed of some simple rules and is easy to implement, but in a simple implementation, it is necessary to check the positional relationship with all individuals, and if the number of individuals increases, it becomes the square. The amount of calculation will increase in proportion. If you want to control many individuals, it is very difficult to implement with CPU. Therefore, take advantage of the powerful parallel computing power of the GPU. A shader program called ComputeShader is provided in Unity to perform such general-purpose calculation (GPGPU) by GPU. The GPU has a special storage area called shared memory that can be used effectively by using ComputeShader. In addition, Unity has an advanced rendering function called GPU instancing, and it is possible to draw large numbers of arbitrary meshes. We will introduce a program that controls and draws a large number of Boid objects using the functions that make use of the GPU's computing power.


== Boids algorithm


A group of simulation algorithms called Boids was developed by Craig Reynolds in 1986 and published the following year in 1987 at ACM SIGGRAPH as a paper entitled "Flocks, Herds, and Schools: A Distributed Behavioral Model".



Reynolds is a herd that each individual modifies its own behavior based on the position and moving direction of other individuals around it by perception such as sight and hearing, resulting in complicated behavior. I will focus on that.



Each individual follows three simple rules of behavior:


===== 1.分離（Separation）


Move to avoid crowding with individuals within a certain distance


===== 2.整列（Alignment）


An individual within a certain distance moves toward the average of the directions they are facing


===== 3.結合（Cohesion）


Move to the average position of individuals within a certain distance



//image[boids-rules][Boids basic rules]{
//}




Following these rules, you can program herd movements by controlling individual movements.


== Sample program

=== Repository


@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming,https://github.com/IndieVisualLab/UnityGraphicsProgramming}



Assets in the sample Unity project in this book/@<strong>{BoidsSimulationOnGPU}In a folder@<strong>{BoidsSimulationOnGPU.unity}Please open the scene data.


=== Execution condition


The program introduced in this chapter uses ComputeShader, GPU instancing.



ComputeShader Works on the following platforms or APIs:

 * Windows and Windows Store apps with DirectX11 or DirectX12 graphics API and shader model 5.0 GPU
 * IOS with MacOS and Metal Graphics API
 * Android, Linux, Windows platforms with Vulkan API
 * The latest OpenGL platform (OpenGL 4.3 on Linux or Windows, OpenGL ES 3.1 on Android). (Note that MacOSX does not support OpenGL 4.3)
 * Console machines commonly used at this stage (Sony PS4, Microsoft Xbox One)



GPU instancing is available on the following platforms or APIs.

 * DirectX 11 and DirectX 12 on Windows
 * OpenGL Core 4.1+/ES3.0+ on Windows, MacOS, Linux, iOS, Android
 * Metal on MacOS and iOS
 * Vulkan for Windows and Android
 * PlayStation 4 and Xbox One
 * WebGL（WebGL 2.0 APIs necessary）



This sample program uses Graphics.DrawMeshInstacedIndirect method. Therefore, Unity version must be 5.6 or later.


== Description of implementation code


This sample program consists of the following code.

 * GPUBoids.cs - Script that controls Compute Shader that simulates Boids
 * Boids.compute - ComputeShader that simulates Boids
 * BoidsRender.cs - C# script that controls the shader that draws the Boids
 * BoidsRender.shader - A shader for drawing objects by GPU instancing



Scripts, material resources etc. are set like this



//image[editor-boids][UnityEditor Settings on]{
//}



=== GPUBoids.cs


This code manages the Boids simulation parameters, ComputeShader that describes the buffers and calculation instructions required for calculation on the GPU.


//emlist[GPUBoids.cs][csharp]{

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class GPUBoids : MonoBehaviour
{
    // Boid data structure
    [System.Serializable]
    struct BoidData 
    {
        public Vector3 Velocity; // 速度
        public Vector3 Position; // 位置
    }
    // Thread group thread size
    const int SIMULATION_BLOCK_SIZE = 256;

    #region Boids Parameters
    // Maximum number of objects
    [Range(256, 32768)]
    public int MaxObjectNum = 16384;

    // Radius with other individuals to which the bond is applied
    public float CohesionNeighborhoodRadius  = 2.0f;
    // Radius of alignment with other individuals
    public float AlignmentNeighborhoodRadius = 2.0f;
    // Radius to other individuals to which separation is applied
    public float SeparateNeighborhoodRadius  = 1.0f;

    // Maximum speed
    public float MaxSpeed        = 5.0f;
    // Maximum steering force
    public float MaxSteerForce   = 0.5f;

    // Weight of force to combine
    public float CohesionWeight  = 1.0f;
    // Force weights to align
    public float AlignmentWeight = 1.0f;
    // Separating force weights
    public float SeparateWeight  = 3.0f;

    // Weight of power to avoid walls
    public float AvoidWallWeight = 10.0f;

    // Center coordinates of wall
    public Vector3 WallCenter = Vector3.zero;
    // Wall size
    public Vector3 WallSize = new Vector3(32.0f, 32.0f, 32.0f);
    #endregion

    #region Built-in Resources
    // Reference of Compute Shader for Boids simulation
    public ComputeShader BoidsCS;
    #endregion

    #region Private Resources
    // A buffer that stores the steering force (Force) of the Boid
    ComputeBuffer _boidForceBuffer;
    // Buffer that stores basic data of Boid (speed, position)
    ComputeBuffer _boidDataBuffer;
    #endregion

    #region Accessors
    // Get the buffer that stores the basic data of Boid
    public ComputeBuffer GetBoidDataBuffer()
    {
        return this._boidDataBuffer != null ? this._boidDataBuffer : null;
    }

    // Get number of objects
    public int GetMaxObjectNum()
    {
        return this.MaxObjectNum;
    }

    // Returns the center coordinates of the simulation area
    public Vector3 GetSimulationAreaCenter()
    {
        return this.WallCenter;
    }

    // Returns the size of the box in the simulation area
    public Vector3 GetSimulationAreaSize()
    {
        return this.WallSize;
    }
    #endregion

    #region MonoBehaviour Functions
    void Start()
    {
        // Initialize buffer
        InitBuffer();
    }

    void Update()
    {
        // just calls run simulation
        Simulation();
    }

    void OnDestroy()
    {
        // Discard buffer
        ReleaseBuffer(); 
    }

    void OnDrawGizmos()
    {
        // Drawing the simulation area in wireframe for debugging
        Gizmos.color = Color.cyan;
        Gizmos.DrawWireCube(WallCenter, WallSize);
    }
    #endregion

    #region Private Functions
    // Initialize buffer
    void InitBuffer()
    {
        // Initialize buffer
        _boidDataBuffer  = new ComputeBuffer(MaxObjectNum, 
            Marshal.SizeOf(typeof(BoidData)));        
        _boidForceBuffer = new ComputeBuffer(MaxObjectNum, 
            Marshal.SizeOf(typeof(Vector3)));

        // Boid Initialize data, Force buffer
        var forceArr = new Vector3[MaxObjectNum];
        var boidDataArr = new BoidData[MaxObjectNum];
        for (var i = 0; i < MaxObjectNum; i++)
        {
            forceArr[i] = Vector3.zero;
            boidDataArr[i].Position = Random.insideUnitSphere * 1.0f;
            boidDataArr[i].Velocity = Random.insideUnitSphere * 0.1f;
        }
        _boidForceBuffer.SetData(forceArr);
        _boidDataBuffer.SetData(boidDataArr);
        forceArr    = null;
        boidDataArr = null;
    }

    // simulation
    void Simulation()
    {
        ComputeShader cs = BoidsCS;
        int id = -1;

        // Find the number of thread groups
        int threadGroupSize = Mathf.CeilToInt(MaxObjectNum 
            / SIMULATION_BLOCK_SIZE);

        // Calculate steering force
        id = cs.FindKernel("ForceCS"); // カーネルIDを取得
        cs.SetInt("_MaxBoidObjectNum", MaxObjectNum);
        cs.SetFloat("_CohesionNeighborhoodRadius",  
            CohesionNeighborhoodRadius);
        cs.SetFloat("_AlignmentNeighborhoodRadius", 
            AlignmentNeighborhoodRadius);
        cs.SetFloat("_SeparateNeighborhoodRadius",  
            SeparateNeighborhoodRadius);
        cs.SetFloat("_MaxSpeed", MaxSpeed);
        cs.SetFloat("_MaxSteerForce", MaxSteerForce);
        cs.SetFloat("_SeparateWeight", SeparateWeight);
        cs.SetFloat("_CohesionWeight", CohesionWeight);
        cs.SetFloat("_AlignmentWeight", AlignmentWeight);
        cs.SetVector("_WallCenter", WallCenter);
        cs.SetVector("_WallSize", WallSize);
        cs.SetFloat("_AvoidWallWeight", AvoidWallWeight);
        cs.SetBuffer(id, "_BoidDataBufferRead", _boidDataBuffer);
        cs.SetBuffer(id, "_BoidForceBufferWrite", _boidForceBuffer);
        cs.Dispatch(id, threadGroupSize, 1, 1); // ComputeShaderを実行

        // Calculate speed and position from steering force
        id = cs.FindKernel("IntegrateCS"); // カーネルIDを取得
        cs.SetFloat("_DeltaTime", Time.deltaTime);
        cs.SetBuffer(id, "_BoidForceBufferRead", _boidForceBuffer);
        cs.SetBuffer(id, "_BoidDataBufferWrite", _boidDataBuffer);
        cs.Dispatch(id, threadGroupSize, 1, 1); // ComputeShaderを実行
    }

    // Free buffer
    void ReleaseBuffer()
    {
        if (_boidDataBuffer != null)
        {
            _boidDataBuffer.Release();
            _boidDataBuffer = null;
        }

        if (_boidForceBuffer != null)
        {
            _boidForceBuffer.Release();
            _boidForceBuffer = null;
        }
    }
    #endregion
}

//}

===== ComputeBuffer initialization


The InitBuffer function declares the buffer used when performing calculations on the GPU.
We will use a class called ComputeBuffer as a buffer to store data for calculation on the GPU. ComputeBuffer is a data buffer that stores data for ComputeShader. You can read and write to the memory buffer on GPU from C# script. Pass the number of elements in the buffer and the size (number of bytes) of each element in the arguments at initialization. You can get the size (number of bytes) of a type by using the Marshal.SizeOf() method. In ComputeBuffer, you can use SetData() to set the array value of an arbitrary structure.

===== Execution of the function described in ComputeShader


In the Simulation function, pass the necessary parameters to ComputeShader and issue the calculation instruction.



The function described in ComputeShader that actually causes the GPU to perform calculations is called the kernel. The execution unit of this kernel is called a thread, and in order to perform parallel calculation processing that conforms to the GPU architecture, any number of them are treated as a group, and they are called a thread group.
Set the product of the number of threads and the number of thread groups to be equal to or greater than the number of Boid object individuals.



The kernel is specified in the ComputeShader script using the #pragma kernel directive. Each ID is assigned to this, and this ID can be obtained from the C# script by using the FindKernel method.



Use the SetFloat method, SetVector method, SetBuffer method, etc. to pass parameters and buffers required for simulation to ComputeShader. You need the kernel ID when setting buffers and textures.



By executing the Dispatch method, a command is issued so that the kernel defined in ComputeShader will be calculated by the GPU. In the argument, specify the kernel ID and the number of thread groups.


=== Boids.compute


Write the calculation instruction to the GPU. There are two kernels, one to calculate the steering force and the other to apply that force and update the speed and position.


//emlist[Boids.compute][computeshader]{

// Specify kernel function
#pragma kernel ForceCS      // 操舵力を計算
#pragma kernel IntegrateCS  // 速度, 位置を計算

// Boid data structure
struct BoidData
{
    float3 velocity; // 速度
    float3 position; // 位置
};

// Thread group thread size
#define SIMULATION_BLOCK_SIZE 256

// Boid data buffer (for reading)
StructuredBuffer<BoidData>   _BoidDataBufferRead;
// Boid data buffer (for reading and writing)
RWStructuredBuffer<BoidData> _BoidDataBufferWrite;
// Boid steering force buffer (for reading)
StructuredBuffer<float3>     _BoidForceBufferRead;
// Boid steering force buffer (for reading and writing)
RWStructuredBuffer<float3>   _BoidForceBufferWrite;

int _MaxBoidObjectNum; // Boidオブジェクト数

float _DeltaTime;      // Time elapsed from the previous frame

float _SeparateNeighborhoodRadius;  // Distance to other individuals to which the separation applies
float _AlignmentNeighborhoodRadius; // Distance to other individuals to which the alignment applies
float _CohesionNeighborhoodRadius;  // Distance from other individuals to which the bond is applied

float _MaxSpeed;        // Maximum speed
float _MaxSteerForce;   // Maximum steering force

float _SeparateWeight;  // Weight when applying separation
float _AlignmentWeight; // Weight when applying alignment
float _CohesionWeight;  // Weight when applying join

float4 _WallCenter;      // Center coordinates of wall
float4 _WallSize;        // Wall size
float  _AvoidWallWeight; // Weight of strength to avoid walls


// Limit vector magnitude
float3 limit(float3 vec, float max)
{
    float length = sqrt(dot(vec, vec)); // 大きさ
    return (length > max && length > 0) ? vec.xyz * (max / length) : vec.xyz;
}

// Returns the opposite force when it hits the wall
float3 avoidWall(float3 position)
{
    float3 wc = _WallCenter.xyz;
    float3 ws = _WallSize.xyz;
    float3 acc = float3(0, 0, 0);
    // x
    acc.x = (position.x < wc.x - ws.x * 0.5) ? acc.x + 1.0 : acc.x;
    acc.x = (position.x > wc.x + ws.x * 0.5) ? acc.x - 1.0 : acc.x;

    // y
    acc.y = (position.y < wc.y - ws.y * 0.5) ? acc.y + 1.0 : acc.y;
    acc.y = (position.y > wc.y + ws.y * 0.5) ? acc.y - 1.0 : acc.y;

    // z
    acc.z = (position.z < wc.z - ws.z * 0.5) ? acc.z + 1.0 : acc.z;
    acc.z = (position.z > wc.z + ws.z * 0.5) ? acc.z - 1.0 : acc.z;

    return acc;
}

// Shared memory Boid data storage
groupshared BoidData boid_data[SIMULATION_BLOCK_SIZE];

// Steering force calculation kernel function
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ForceCS
(
    uint3 DTid : SV_DispatchThreadID, // Unique ID for the entire thread
    uint3 Gid : SV_GroupID, // Group ID
    uint3 GTid : SV_GroupThreadID, // Thread ID within group
    uint  GI : SV_GroupIndex // One-dimensional SV_GroupThreadID 0-255
)
{
    const unsigned int P_ID = DTid.x; // Own ID
    float3 P_position = _BoidDataBufferRead[P_ID].position; // Own position
    float3 P_velocity = _BoidDataBufferRead[P_ID].velocity; // Own speed

    float3 force = float3(0, 0, 0); // Initialize steering force

    float3 sepPosSum = float3(0, 0, 0); // Position addition variable for separation calculation
    int sepCount = 0; // Variable for counting the number of other individuals calculated for separation

    float3 aliVelSum = float3(0, 0, 0); // Speed ​​addition variable for alignment calculation
    int aliCount = 0; // A variable for counting the number of other individuals calculated for alignment

    float3 cohPosSum = float3(0, 0, 0); // Position addition variable for join calculation
    int cohCount = 0; // A variable for counting the number of other individuals calculated for the combination

    // SIMULATION_BLOCK_SIZE（Execution for each group thread number) (Run for the number of groups)
    [loop]
    for (uint N_block_ID = 0; N_block_ID < (uint)_MaxBoidObjectNum;
        N_block_ID += SIMULATION_BLOCK_SIZE)
    {
        // Boid data for SIMULATION_BLOCK_SIZE is stored in shared memory
        boid_data[GI] = _BoidDataBufferRead[N_block_ID + GI];

        // All group share access is complete,
        // Until all threads in the group reach this call
        // Block execution of all threads in group
        GroupMemoryBarrierWithGroupSync();

        // Calculation with other individuals
        for (int N_tile_ID = 0; N_tile_ID < SIMULATION_BLOCK_SIZE; 
            N_tile_ID++)
        {
            // Location of other individuals
            float3 N_position = boid_data[N_tile_ID].position;
            // The speed of other individuals
            float3 N_velocity = boid_data[N_tile_ID].velocity;

            // Difference in position between yourself and other individuals
            float3 diff = P_position - N_position;
            // Distance between yourself and other individuals
            float  dist = sqrt(dot(diff, diff));   

            // --- Separate（Separation） ---
            if (dist > 0.0 && dist <= _SeparateNeighborhoodRadius)
            {
                // Vector from other individuals' positions to themselves
                float3 repulse = normalize(P_position - N_position);
                // Divide by the distance between itself and the position of another individual (the longer the distance, the smaller the effect)
                repulse /= dist;
                sepPosSum += repulse; // Addition
                sepCount++;           // Population count
            }

            // --- Alignment（Alignment） ---
            if (dist > 0.0 && dist <= _AlignmentNeighborhoodRadius)
            {
                aliVelSum += N_velocity; // 加算
                aliCount++;              // 個体数カウント
            }

            // --- Combine（Cohesion） ---
            if (dist > 0.0 && dist <= _CohesionNeighborhoodRadius)
            {
                cohPosSum += N_position; // Addition
                cohCount++;              // Population count
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // Steering force (separation)
    float3 sepSteer = (float3)0.0;
    if (sepCount > 0)
    {
        sepSteer = sepPosSum / (float)sepCount;     // Find the average
        sepSteer = normalize(sepSteer) * _MaxSpeed; // Adjust to maximum speed
        sepSteer = sepSteer - P_velocity;           // Calculate steering force
        sepSteer = limit(sepSteer, _MaxSteerForce); // Limit steering force
    }

    // 操舵力（整列）
    float3 aliSteer = (float3)0.0;
    if (aliCount > 0)
    {
        aliSteer = aliVelSum / (float)aliCount; // Find the average velocity of close individuals
        aliSteer = normalize(aliSteer) * _MaxSpeed; // Adjust to maximum speed
        aliSteer = aliSteer - P_velocity;           // Calculate steering force
        aliSteer = limit(aliSteer, _MaxSteerForce); // Limit steering force
    }
    // Steering power (combined)
    float3 cohSteer = (float3)0.0;
    if (cohCount > 0)
    {
        // / Find the average of the positions of close individuals
        cohPosSum = cohPosSum / (float)cohCount;
        cohSteer = cohPosSum - P_position; // Find vector toward average position
        cohSteer = normalize(cohSteer) * _MaxSpeed; // Adjust to maximum speed
        cohSteer = cohSteer - P_velocity;           // Calculate steering force
        cohSteer = limit(cohSteer, _MaxSteerForce); // Limit steering force
    }
    force += aliSteer * _AlignmentWeight; // Add force to align with steering force
    force += cohSteer * _CohesionWeight;  // Add force to the steering force
    force += sepSteer * _SeparateWeight;  // Add a separating force to the steering force

    _BoidForceBufferWrite[P_ID] = force; // writing
}

// Kernel function for velocity and position calculation
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void IntegrateCS
(
    uint3 DTid : SV_DispatchThreadID // Unique ID for the entire thread
)
{
    const unsigned int P_ID = DTid.x; // Get index

    BoidData b = _BoidDataBufferWrite[P_ID]; // Read current Boid data
    float3 force = _BoidForceBufferRead[P_ID]; // Read steering force

    // Gives power to repel when approaching a wall
    force += avoidWall(b.position) * _AvoidWallWeight; 

    b.velocity += force * _DeltaTime; // Apply steering force to speed
    b.velocity = limit(b.velocity, _MaxSpeed); // Speed ​​limit
    b.position += b.velocity * _DeltaTime; // Update position

    _BoidDataBufferWrite[P_ID] = b; // Write the calculation result
}

//}

==== Steering force calculation


The ForceCS kernel calculates steering force.


===== Utilization of shared memory


Variables with the storage modifier groupshared will now be written to shared memory.
Although the shared memory cannot write a large amount of data, it is located close to the registers and can be accessed very fast.
This shared memory can be shared within a thread group. SIMULATION_BLOCK_SIZE's worth of information about other individuals can be collectively written to the shared memory so that they can be read at high speed within the same thread group, making efficient calculation considering the positional relationship with other individuals. I will go on a regular basis.



//image[gpu-architecture][GPU basic architecture]{
//}



====== GroupMemoryBarrierWithGroupSync()

When accessing the data written in the shared memory, it is necessary to write the GroupMemoryBarrierWithGroupSync() method to synchronize the processing of all threads in the thread group.
GroupMemoryBarrierWithGroupSync() blocks execution of all threads in the thread group until all threads in the group reach this call. This will ensure that all threads in the thread group have properly initialized the boid_data array.

===== Calculate steering force by distance from other individuals

====== Separation


If there is an individual closer than the specified distance, the vector from that individual's position to its own position is calculated and normalized. By dividing the vector by the distance value, it is weighted so that it is more avoided when it is close and smaller when it is far, and it is added as a force to prevent collision with other individuals. When the calculation with all the individuals is completed, use that value to calculate the steering force from the relationship with the current speed.


====== Alignment


If there is an individual closer than the specified distance, the velocity (Velocity) of that individual is added together, and at the same time, the number of individuals is counted, and with those values, the velocity of the closer individual (that is, the direction in which it is facing) Find the average of. When the calculation with all the individuals is completed, use that value to calculate the steering force from the relationship with the current speed.


====== Cohesion


If there is an individual closer than the specified distance, the position of that individual is added, and at the same time, the number of that individual is counted, and the average (center of gravity) of the positions of close individuals is calculated from these values. In addition, the vector going to it is calculated, and the steering force is calculated from the relationship with the current speed.


===== Update velocity and position of individual Boids


The IntegrateCS kernel updates the speed and position of the Boid based on the steering force obtained by ForceCS().
AvoidWall tries to stay outside the specified area by applying a reverse force when trying to get out of the specified area.


=== BoidsRender.cs


In this script, the result obtained by the Boids simulation is drawn with the specified mesh.


//emlist[BoidsRender.cs][csharp]{

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// Guaranteed that the GPU Boids component is attached to the same GameObject
[RequireComponent(typeof(GPUBoids))]
public class BoidsRender : MonoBehaviour
{
    #region Paremeters
    // Scale of Boids object to draw
    public Vector3 ObjectScale = new Vector3(0.1f, 0.2f, 0.5f);
    #endregion

    #region Script References
    // GPUBoids script reference
    public GPUBoids GPUBoidsScript;
    #endregion

    #region Built-in Resources
    // Reference to the mesh to draw
    public Mesh InstanceMesh;
    // Material reference for drawing
    public Material InstanceRenderMaterial;
    #endregion

    #region Private Variables
    // Argument for GPU instancing (for transfer to ComputeBuffer)
    // Number of indexes per instance, number of instances,
    // Start index position, base vertex position, instance start position
    uint[] args = new uint[5] { 0, 0, 0, 0, 0 };
    // Argument buffer for GPU instancing
    ComputeBuffer argsBuffer;
    #endregion

    #region MonoBehaviour Functions
    void Start ()
    {
        // Initialize the argument buffer
        argsBuffer = new ComputeBuffer(1, args.Length * sizeof(uint), 
            ComputeBufferType.IndirectArguments);
    }

    void Update ()
    {
        // Instantiating mesh
        RenderInstancedMesh();
    }

    void OnDisable()
    {
        // Free argument buffer
        if (argsBuffer != null)
            argsBuffer.Release();
        argsBuffer = null;
    }
    #endregion

    #region Private Functions
    void RenderInstancedMesh()
    {
        // The drawing material is Null, or the GPUBoids script is Null,
        // Or if GPU instancing is not supported, do nothing
        if (InstanceRenderMaterial == null || GPUBoidsScript == null || 
            !SystemInfo.supportsInstancing)
            return;

        // Get the index number of the specified mesh
        uint numIndices = (InstanceMesh != null) ? 
            (uint)InstanceMesh.GetIndexCount(0) : 0;
        // Set the number of mesh indexes
        args[0] = numIndices; 
        // Set the number of instances
        args[1] = (uint)GPUBoidsScript.GetMaxObjectNum(); 
        argsBuffer.SetData(args); // Set in buffer

        // Set the buffer that stores the Boid data in the material
        InstanceRenderMaterial.SetBuffer("_BoidDataBuffer", 
            GPUBoidsScript.GetBoidDataBuffer());
        // Boid object scale set
        InstanceRenderMaterial.SetVector("_ObjectScale", ObjectScale);
        // Bounding area defined
        var bounds = new Bounds
        (
            GPUBoidsScript.GetSimulationAreaCenter(), // 中心
            GPUBoidsScript.GetSimulationAreaSize()    // サイズ
        );
        // GPU instancing and drawing mesh
        Graphics.DrawMeshInstancedIndirect
        (
            InstanceMesh,           // The mesh to instantiate
            0,                      // submesh index
            InstanceRenderMaterial, // Material to draw
            bounds,                 // Realm
            argsBuffer              // Argument buffer for GPU instancing
        );
    }
    #endregion
}

//}

==== GPU instancing


If you want to draw a large number of identical meshes, creating a GameObject one by one will increase the draw call and the drawing load will increase. In addition, the cost of transferring the calculation result of ComputeShader to CPU memory is high, and if you want to perform high-speed processing, it is necessary to pass the calculation result of GPU to the shader for drawing as it is and perform drawing processing. If you use Unity's GPU instancing, you can draw a large number of same meshes at high speed with few draw calls without generating unnecessary GameObjects.


====== Graphics.DrawMeshInstancedIndirect()


This script uses the Graphics.DrawMeshInstancedIndirect method to draw the mesh by GPU instancing.
In this method, you can pass the number of mesh indexes and the number of instances as ComputeBuffer. This is useful if you want to read all instance data from the GPU.



In Start(), the argument buffer for this GPU instancing is initialized. Specify @<b>{ComputeBufferType.IndirectArguments} as the third argument of the constructor at initialization.



RenderInstancedMesh() executes mesh drawing by GPU instancing. Boid data (velocity, position array) obtained by the Boids simulation is passed to the material for rendering InstanceRenderMaterial by the SetBuffer method.



Graphics.DrawMeshInstancedIndrectメソッドには、インスタンシングするメッシュ、submeshのインデックス、描画用マテリアル、境界データ、また、インスタンス数などのデータを格納したバッファを引数に渡します。



This method should normally be called within Update().


=== BoidsRender.shader


A shader for drawing corresponding to the Graphics.DrawMeshInstancedIndrect method.


//emlist[BoidsRender.shader][hlsl]{

Shader "Hidden/GPUBoids/BoidsRender"
{
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _MainTex ("Albedo (RGB)", 2D) = "white" {}
        _Glossiness ("Smoothness", Range(0,1)) = 0.5
        _Metallic ("Metallic", Range(0,1)) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM
        #pragma surface surf Standard vertex:vert addshadow
        #pragma instancing_options procedural:setup

        struct Input
        {
            float2 uv_MainTex;
        };
        // Boid structure
        struct BoidData
        {
            float3 velocity; // 速度
            float3 position; // 位置
        };

        #ifdef UNITY_PROCEDURAL_INSTANCING_ENABLED
        // Boid data structure buffer
        StructuredBuffer<BoidData> _BoidDataBuffer;
        #endif

        sampler2D _MainTex; // texture

        half   _Glossiness; // Gloss
        half   _Metallic;   // Metal properties
        fixed4 _Color;      // Color

        float3 _ObjectScale; // Boid Object scale

        // Convert Euler angles (radians) to rotation matrix
        float4x4 eulerAnglesToRotationMatrix(float3 angles)
        {
            float ch = cos(angles.y); float sh = sin(angles.y); // heading
            float ca = cos(angles.z); float sa = sin(angles.z); // attitude
            float cb = cos(angles.x); float sb = sin(angles.x); // bank

            // RyRxRz (Heading Bank Attitude)
            return float4x4(
                ch * ca + sh * sb * sa, -ch * sa + sh * sb * ca, sh * cb, 0,
                cb * sa, cb * ca, -sb, 0,
                -sh * ca + ch * sb * sa, sh * sa + ch * sb * ca, ch * cb, 0,
                0, 0, 0, 1
            );
        }

        // Vertex shader
        void vert(inout appdata_full v)
        {
            #ifdef UNITY_PROCEDURAL_INSTANCING_ENABLED

            // Get Boid data from instance ID
            BoidData boidData = _BoidDataBuffer[unity_InstanceID]; 

            float3 pos = boidData.position.xyz; // Get Boid position
            float3 scl = _ObjectScale;          // Get Boid Scale

            // Define a matrix to transform from object coordinates to world coordinates
            float4x4 object2world = (float4x4)0; 
            // Substitute scale value
            object2world._11_22_33_44 = float4(scl.xyz, 1.0);
            // Calculate rotation about Y axis from speed
            float rotY = 
                atan2(boidData.velocity.x, boidData.velocity.z);
            // Calculate rotation about X axis from speed
            float rotX = 
                -asin(boidData.velocity.y / (length(boidData.velocity.xyz)
                + 1e-8)); // 0 division prevention
            // Find rotation matrix from Euler angles (radians)
            float4x4 rotMatrix = 
                eulerAnglesToRotationMatrix(float3(rotX, rotY, 0));
            // Apply rotation to matrix
            object2world = mul(rotMatrix, object2world);
            // Apply position (translation) to matrix
            object2world._14_24_34 += pos.xyz;

            // Coordinate conversion of vertices
            v.vertex = mul(object2world, v.vertex);
            // Convert normal to coordinates
            v.normal = normalize(mul(object2world, v.normal));
            #endif
        }

        void setup()
        {
        }

        // Surface shader
        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;
        }
        ENDCG
    }
    FallBack "Diffuse"
}

//}


#pragma surface surf Standard vertex:vert addshadow
In this part, surf() is specified as the surface shader, Standard is specified as the lighting model, and vert() is specified as the custom vertex shader.



You can tell Unity to generate an additional variant for when you use the Graphics.DrawMeshInstancedIndirect method by writing procedural:FunctionName in the #pragma instancing_options directive, and specify it in FunctionName at the beginning of the vertex shader stage. The called function will be called.
Official sample（https://docs.unity3d.com/ScriptReference/
Graphics.DrawMeshInstancedIndirect.html）Looking at etc., in this function, unity_ObjectToWorld matrix, unity_WorldToObject matrix is ​​rewritten based on the position, rotation and scale of each instance, but in this sample program, data of Boids is received in the vertex shader, I am converting the coordinates of vertices and normals (I am not sure if it is good ...).
Therefore, nothing is written in the specified setup function.


==== Get the Boid data for each instance with the vertex shader and convert the coordinates


In Vertex Shader, describe the processing to be performed on the vertices of the mesh passed to the shader.



You can get the unique ID for each instance by unity_InstanceID. By specifying this ID as the index of the array of StructuredBuffer that is declared as a buffer of Boid data, you can get Boid data unique to each instance.


==== Ask for rotation


From the velocity data of the Boid, calculate the rotation value so as to face the traveling direction.
In order to handle it intuitively, rotation is expressed by Euler angles.
When the Boid is regarded as a flying object, the rotations of the three axes with respect to the object are called pitch, yaw, and roll, respectively.



//image[roll-pitch-yaw][Axis and rotation designation]{
//}




First, from the velocity about the Z-axis and the velocity about the X-axis, yaw (which direction it is facing with respect to the horizontal plane) is obtained using the atan2 method that returns the arctangent.



//image[arctan][Relationship between speed and angle (yaw)]{
//}




Next, the asin method that returns the arc sine (arc sine) is used to find the pitch (upward and downward inclination) from the ratio of the speed and the speed about the Y axis. When the Y-axis speed is small among the speeds for each axis, the amount of rotation is weighted so that the Y-axis speed is small and the level is kept horizontal.



//image[arcsin][Relationship between speed and angle (pitch)]{
//}



==== Compute matrix applying Boid transform


Coordinate conversion processing such as movement, rotation, and scaling can be expressed together in a single matrix.
Define a 4x4 matrix object2world.


===== Scale


First, substitute the scale value.
For each XYZ axis @<m>{\rm S_x S_y S_z {\}} The matrix S that scales only by is expressed as follows.



//texequation{
\rm
S=
\left(
\begin{array}{cccc}
\rm S_x & 0 & 0 & 0 \\
0 & \rm S_y & 0 & 0 \\
0 & 0 & \rm S_z & 0 \\
0 & 0 & 0 & 1
\end{array}
\right)
//}



HLSL float4x4 type variables can specify a particular element of the matrix using swizzles like ._11_22_33_44.
By default, the components are arranged as follows.

//table[tbl2][]{
11	12	13	14
-----------------
21	22	23	24
31	32	33	34
41	42	43	44
//}


Here, substitute the values ​​of the scale of XYZ for 11, 22, 33, and 1 for 44.


===== rotation


Then apply rotation.
Rotation about each XYZ axis @<m>{\rm R_x R_y R_z {\}} is expressed as a matrix,



//texequation{
\rm
R_x(\phi)=
\left(
\begin{array}{cccc}
1 & 0 & 0 & 0 \\
0 & \rm cos(\phi) & \rm -sin(\phi) & 0 \\
0 & \rm sin(\phi) & \rm cos(\phi) & 0 \\
0 & 0 & 0 & 1
\end{array}
\right)
//}



//texequation{
\rm
R_y(\theta)=
\left(
\begin{array}{cccc}
\rm cos(\theta) & 0 & \rm sin(\theta) & 0 \\
0 & 1 & 0 & 0 \\
\rm -sin(\theta) & 0 & \rm cos(\theta) & 0 \\
0 & 0 & 0 & 1
\end{array}
\right)
//}



//texequation{
\rm
R_z(\psi)=
\left(
\begin{array}{cccc}
\rm cos(\psi) & \rm -sin(\psi) & 0 & 0 \\
\rm sin(\psi) & \rm cos(\psi) & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{array}
\right)
//}



This is combined into a matrix. At this time, the behavior at the time of rotation changes depending on the order of the rotation axis to be combined, but if combined in this order, it should be similar to Unity's standard rotation.



//image[synth-euler2matrix][Composition of rotation matrix]{
//}




Apply rotation by finding the product of the resulting rotation matrix and the scaled matrix above.


===== Translation


Then apply the translation.
On each axis,@<m>{\rm T_x T_y T_z {\}} When translated, the matrix is ​​represented as



//texequation{
\rm T=
\left(
\begin{array}{cccc}
1 & 0 & 0 & \rm T_x \\
0 & 1 & 0 & \rm T_y \\
0 & 0 & 1 & \rm T_z \\
0 & 0 & 0 & 1
\end{array}
\right)
//}



This translation can be applied by adding the position data for the XYZ axes to the 14, 24 and 34 components.



By applying the matrix obtained by these calculations to the vertices and normals, the Boid transform data is reflected.


=== Drawing result


I think that an object that moves like a flock like this is drawn.



//image[result][Execution result]{
//}



== Summary


The implementation introduced in this chapter uses the minimum Boids algorithm, but it also has different characteristics such as a large group or several small colonies even if the parameters are adjusted. I think it will show a movement. In addition to the basic rules of behavior shown here, there are other rules to consider. For example, if this is a school of fish, it will naturally escape if a foreign enemy preying on them appears, and if there are obstacles such as terrain, the fish will avoid hitting it. In terms of vision, the field of view and accuracy differ depending on the species of animal, and I think that if you exclude other individuals outside the field of view from the calculation process, you will get closer to the actual one. The characteristics of movement also change depending on the environment such as whether you fly in the sky, move in water, or move on land, and the characteristics of the motor organs for locomotion. You should also pay attention to individual differences.



Parallel processing by GPU can calculate many individuals compared to the calculation by CPU, but basically, it does brute force calculation with other individuals, so it cannot be said that the calculation efficiency is very good. To do this, the cost of computation is increased by increasing the efficiency of the neighboring individual search, such as registering individuals in regions divided by grids or blocks according to their positions and performing calculation processing only for the individuals that exist in adjacent regions. Can be suppressed.



There is plenty of room for improvement in this way, and by applying proper implementation and behavior rules, it is possible to express more beautiful, powerful, dense and tasting group movements. I want to be able to do it.


== reference
 * Boids Background and Update - https://www.red3d.com/cwr/boids/
 * THE NATURE OF CODE - http://natureofcode.com/
 * Real-Time Particle Systems on the GPU in Dynamic Environments - http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2013/02/Chapter7-Drone-Real-Time@<b>{Particle}Systems@<b>{On}The_GPU.pdf
 * Practical Rendering and Computation with Direct3D 11 - https://dl.acm.org/citation.cfm?id=2050039
 * GPU 並列図形処理入門 - http://gihyo.jp/book/2014/978-4-7741-6304-8

