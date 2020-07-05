= ComputeShader: Getting Started


A simple explanation of how to use ComputeShader (hereafter "compute shader" if needed) in Unity.
Compute shader is used to execute a large number of operations at high speed by parallelizing simple processing using GPU.
It also delegates processing to the GPU, but is characterized by a difference from the normal rendering pipeline.
It is often used in CG to represent the movement of a large number of particles.

Some of the content that follows this chapter also uses compute shaders,
Knowledge of compute shaders is required to read through them.

Here, in learning Compute Shader,
We'll use two simple samples to get you started with the very first step.
These do not cover all aspects of compute shaders, so make sure to supplement the information as needed.

In Unity it's called ComputeShader,
Similar technologies include OpenCL, DirectCompute, and CUDA.
The basic concepts are similar and are very closely related to DirectCompute(DirectX).
If you need concepts around the architecture or more details,
I think it would be good to collect information on these items as well.

The sample in this chapter is "SimpleComputeShader" from @<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming}.
== Kernel, thread, group concept


//image[primerofcomputeshader01][Images of kernels, threads and groups][scale=1]

Handled by Compute Shaders before describing the specific implementation @<b>{kernel(Kernel)}、
@<b>{thread(Thread)}、@<b>{Group(Group)} It is necessary to explain the concept of.


@<b>{kernel} What is、GPU Refers to a single operation performed by
Treated as a function in code (equivalent to the kernel in the sense of general system terminology).

@<b>{thread}Is the unit that executes the kernel. One thread runs one kernel.
Compute shaders allow the kernel to run concurrently in multiple threads simultaneously.
Threads are specified in three dimensions (x, y, z).

For example,(4, 1, 1) Nara 4 * 1 * 1 = 4 One thread runs at the same time.
(2, 2, 1) Then2 * 2 * 1 = 4 One thread runs at the same time.
The same four threads are executed, but in some cases it may be more efficient to specify threads in two dimensions like the latter.
This will be explained later. For the time being, it is necessary to realize that the number of threads is specified in three dimensions.

Finally@<b>{group}Is the unit of execution of a thread. Also, the thread that a group executes is@<b>{Group thread}Is called.
For example, a group has(4, 1, 1) Suppose you have a thread. This group 2 When there are two groups,(4, 1, 1) Has a thread of.

Groups, like threads, are specified in three dimensions. For example, when a (2, 1, 1) group runs a kernel that runs with (4, 4, 1) threads,
The number of groups is 2 * 1 * 1 = 2. The two groups will each have 4 * 4 * 1 = 16 threads.
Therefore, the total number of threads is 2 * 16 = 32.


== Sample (1): Get the result calculated by GPU


Sample (1) "SampleScene_Array" deals with how to execute an appropriate calculation with a compute shader and get the result as an array.
The sample includes the following operations:

 * Use the compute shader to process multiple data and get the result.
 * Implement multiple functions in the compute shader and use them properly.
 * Pass values ​​from the script (CPU) to the compute shader (GPU).

The execution result of sample (1) is as follows. Check the operation while reading the source code, since it is only the debug output.

//image[primerofcomputeshader03][sample (1) Execution result of][scale=1]


=== Compute shader implementation


From here, we will proceed with the explanation using a sample as an example.
It's very short, so it's a good idea to go through the Compute Shader implementation first.
The basic configuration consists of function definitions, function implementations, buffers, and optionally variables.

//emlist[SimpleComputeShader_Array.compute]{
#pragma kernel KernelFunction_A
#pragma kernel KernelFunction_B

RWStructuredBuffer<int> intBuffer;
float floatValue;

[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * floatValue;
}

[numthreads(4, 1, 1)]
void KernelFunction_B(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] += 1;
}
//}

as a feature,@<b>{numthreads} Attributes,@<b>{SV_GroupID} There are semantics etc.,
This will be discussed later.


=== Kernel definition


As I explained earlier, aside from the exact definition,@<b>{The kernel is a piece of work performed on the GPU and is treated as a function in code.}
Multiple kernels can be implemented in one compute shader.

In this example, the kernel is @<code>{KernelFunction_A} No @<code>{KernelFunction_B} The function corresponds to the kernel.
Also, the function treated as a kernel is @<code>{#pragma kernel} Use to define.
This distinguishes it from the kernel and other functions.

A unique index is given to the kernel to identify any one of the defined kernels.
Index is @<code>{#pragma kernel} They are given as 0, 1 …from the top in the order defined by.


=== Preparing buffers and variables


Create @<b>{buffer area} to save the result executed by Compute Shader.
Sample variables @<code>{RWStructuredBuffer<int> intBuffer}} Is equivalent to this.

Also script (CPU) If you want to give any value from the side、一Prepare variables as in general CPU programming.
この例では変数 @<code>{intValue} Is equivalent to this, and passes the value from the script.


=== numthreads The number of execution threads by


@<b>{numthreads} Attributes (Attribute) Is the kernel (function) Specifies the number of threads to execute.
To specify the number of threads,(x, y, z) Specify with, for example (4, 1, 1) Will run the kernel with 4 * 1 * 1 = 4 threads.
(2, 2, 1) Nara 2 * 2 * 1 = 4 Run the kernel in a thread.
Both are executed with 4 threads, but the difference and usage will be described later.


=== Kernel (function) arguments


There are restrictions on the arguments that can be set in the kernel, and the degree of freedom is extremely low compared to general CPU programming.

The value following the argument is called @<code>{Semantics}, and in this example @<code>{groupID :SV_GroupID} and @<code>{groupThreadID :SV_GroupThreadID} are set. Semantics are just to show what kind of value the argument is, and cannot be changed to any other name.

The argument name (variable name) can be freely defined, but it is necessary to set one of the semantics defined when using the compute shader.
In other words, it is not possible to implement it by defining an argument of any type and referencing it in the kernel.
The argument that can be referred to in the kernel is to select from the defined limited ones.

@<code>{SV_GroupID} Indicates in which group the thread executing the kernel is running (x, y, z).
@<code>{SV_GroupThreadID} Is the (x, y, z) number of the thread that runs the kernel in the group.

例えば (4, 4, 1) In a group of(2, 2, 1) When executing the thread of
@<code>{SV_GroupID} Is (0 ~ 3, 0 ~ 3, 0) Returns the value of.
@<code>{SV_GroupThreadID} Is (0 ~ 1, 0 ~ 1, 0) Returns the value of.

In addition to the semantics set in the sample, there are other semantics starting from @<code>{SV_~}, which you can use,
I will omit the explanation here. I think it's better to read it once you understand the behavior of the compute shader.

 * SV_GroupID - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/ja-jp/library/ee422449(v=vs.85).aspx} 
 ** You can see the different SV~ semantics and their values.


=== Contents of kernel (function) processing


In the sample, the thread number is sequentially assigned to the prepared buffer.
@<code>{groupThreadID} Is given the thread number to run in a group.
This kernel runs with (4, 1, 1) threads, so @<code>{groupThreadID} is given (0 ~ 3, 0, 0).

//emlist[SimpleComputeShader_Array.compute]{
[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * intValue;
}
//}

This sample runs this thread in a group of (1, 1, 1) (from the script below).
That is, run only one group, which contains 4 * 1 * 1 threads.
Make sure @<code>{groupThreadID.x} is given a value between 0 and 3 as a result.

*In this example, @<code>{groupID} is not used, but like threads, the number of groups specified in 3 dimensions is given.
Please try using it to check the behavior of the compute shader, such as by substituting it.

=== Run Compute Shader from script


Execute the implemented compute shader from the script. The items required on the script side are as follows.

 * Reference to Compute Shader | @<code>{comuteShader}
 * Index of kernel to execute | @<code>{kernelIndex_KernelFunction_A, B}
 * A buffer to save the execution result of compute shader | @<code>{intComputeBuffer}

//emlist[SimpleComputeShader_Array.cs]{
public ComputeShader computeShader;
int kernelIndex_KernelFunction_A;
int kernelIndex_KernelFunction_B;
ComputeBuffer intComputeBuffer;

void Start()
{
    this.kernelIndex_KernelFunction_A
        = this.computeShader.FindKernel("KernelFunction_A");
    this.kernelIndex_KernelFunction_B
        = this.computeShader.FindKernel("KernelFunction_B");

    this.intComputeBuffer = new ComputeBuffer(4, sizeof(int));
    this.computeShader.SetBuffer
        (this.kernelIndex_KernelFunction_A,
         "intBuffer", this.intComputeBuffer);

    this.computeShader.SetInt("intValue", 1);
    …
//}


=== Get the index of the kernel to execute


In order to execute a certain kernel, index information for specifying the kernel is required.
Index is @<code>{#pragma kernel} It is given as 0, 1… from the top in the order defined by
From the script side @<code>{FindKernel} It's better to use a function.

//emlist[SimpleComputeShader_Array.cs]{
this.kernelIndex_KernelFunction_A
    = this.computeShader.FindKernel("KernelFunction_A");

this.kernelIndex_KernelFunction_B
    = this.computeShader.FindKernel("KernelFunction_B");
//}


=== Create a buffer to save the calculation result


Prepare the buffer area to save the calculation result by Compute Shader (GPU) on the CPU side.
Unity Then @<code>{ComputeBuffer} Is defined as

//emlist[SimpleComputeShader_Array.cs]{
this.intComputeBuffer = new ComputeBuffer(4, sizeof(int));
this.computeShader.SetBuffer
    (this.kernelIndex_KernelFunction_A, "intBuffer", this.intComputeBuffer);
//}

@<code>{ComputeBuffer} To (1) The size of the area to save,
(2) Specify the size per unit of the data to be saved and initialize it.
There are 4 areas of size int provided here.
This is because the compute shader execution result is saved as int[4].
Resize as needed.

Then, when (1) which kernel implemented in the compute shader executes,
(2) Specify which GPU's buffer to use, and (3) specify which CPU's buffer.
In this example,(1) @<code>{KernelFunction_A} Referenced when is executed,
(2) @<code>{intBuffer} The buffer area is (3) @<code>{intComputeBuffer} Equivalent to.


=== Pass value from script to compute shader


//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.SetInt("intValue", 1);
//}

Depending on what you want to process, you may want to pass a value from the script (CPU) side to the compute shader (GPU) side for reference.
Values ​​of most types can be set to variables inside the compute shader using @<code>{ComputeShader.Set~}.
At this time, the variable name of the argument set in the argument and the variable name defined in the compute shader must match.
In this example we are passing 1 for @<code>{intValue}.

=== Run Compute Shader


The kernel implemented (defined) in the compute shader executes with the @<code>{ComputeShader.Dispatch} method.
Runs the kernel with the specified index in the specified number of groups. The number of groups is specified by X * Y * Z. In this sample, 1 * 1 * 1 = 1 group.

//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.Dispatch
    (this.kernelIndex_KernelFunction_A, 1, 1, 1);

int[] result = new int[4];

this.intComputeBuffer.GetData(result);

for (int i = 0; i < 4; i++)
{
    Debug.Log(result[i]);
}
//}

The execution result of the compute shader (kernel) is obtained with @<code>{ComputeBuffer.GetData}.


=== Confirmation of execution result (A)


Check the implementation on the compute shader side again.
This sample runs the following kernels in a 1 * 1 * 1 = 1 group.
The threads are 4 * 1 * 1 = 4 threads. Also, 1 is given to @<code>{intValue} from the script.

//emlist[SimpleComputeShader_Array.compute]{
[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * intValue;
}
//}


@<code>{groupThreadID(SV_GroupThreadID)} Is
Now it has a value that tells how many threads this group is running in the group, so
In this example, (0 ~ 3, 0, 0) will be entered. So @<code>{groupThreadID.x} is 0-3.
In other words, @<code>{intBuffer[0] = 0} to @<code>{intBuffer[3] = 3} will be executed in parallel.


=== Run a different kernel (B)

When running different kernels implemented in one compute shader, specify the index of another kernel.
This example executes @<code>{KernelFunction_A} and then @<code>{KernelFunction_B}.
Furthermore, the buffer area used by @<code>{KernelFunction_A} is also used by @<code>{KernelFunction_B}.

//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.SetBuffer
(this.kernelIndex_KernelFunction_B, "intBuffer", this.intComputeBuffer);

this.computeShader.Dispatch(this.kernelIndex_KernelFunction_B, 1, 1, 1);

this.intComputeBuffer.GetData(result);

for (int i = 0; i < 4; i++)
{
    Debug.Log(result[i]);
}
//}


=== Confirmation of execution result (B)


@<code>{KernelFunction_B} Executes code similar to the following:
このとき @<code>{intBuffer} は @<code>{KernelFunction_A} Note that we still specify the one used in.

//emlist[SimpleComputeShader_Array.compute]{
RWStructuredBuffer<int> intBuffer;

[numthreads(4, 1, 1)]
void KernelFunction_B
(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] += 1;
}
//}

In this sample, @<code>{KernelFunction_A} By @<code>{intBuffer} 0 to 3 are given in order.
So after executing @<code>{KernelFunction_B}, make sure the value is between 1 and 4.


=== Discard buffer


You need to explicitly destroy the ComputeBuffer when you are done using it.

//emlist[SimpleComputeShader_Array.cs]{
this.intComputeBuffer.Release();
//}


=== Problems not resolved in sample (1)


The intention of specifying multidimensional threads or groups is not covered in this sample.
For example, a (4, 1, 1) thread and a (2, 2, 1) thread both run 4 threads,
These two have the meaning to use properly. This is demonstrated in the sample (2) that follows.


== Sample (2): Make GPU operation result texture


Sample (2) In "SampleScene_Texture", the calculation result of the compute shader is obtained as a texture.
The sample includes the following operations:

 * Write information to texture using compute shader.
 * Effectively utilize multi-dimensional (two-dimensional) threads.

The execution result of sample (2) is as follows. Generates a horizontal and vertical gradient texture.

//image[primerofcomputeshader04][Result of sample (2)][scale=1]


=== Kernel implementation


See sample for full implementation. In this sample, the following code is executed by the compute shader.
Notice that the kernel runs in a multidimensional thread. Since it is (8, 8, 1), there are 8 * 8 * 1 = 64 threads per group.
Another big change is that the calculation result is saved in @<code>{RWTexture2D<float4>}.

//emlist[SimpleComputeShader_Texture.compute]{
RWTexture2D<float4> textureBuffer;

[numthreads(8, 8, 1)]
void KernelFunction_A(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float width, height;
    textureBuffer.GetDimensions(width, height);

    textureBuffer[dispatchThreadID.xy]
        = float4(dispatchThreadID.x / width,
                 dispatchThreadID.x / width,
                 dispatchThreadID.x / width,
                 1);
}
//}


=== Special arguments SV_DispatchThreadID


sample (1) では @<code>{SV_DispatchThradID} I didn't use semantics.
It's a little complicated,@<b>{"Where is the thread that runs a kernel located among all threads?" (x,y,z) 」}Is shown.

@<code>{SV_DispathThreadID} は、@<code>{SV_Group_ID * numthreads + SV_GroupThreadID} The value calculated by.
@<code>{SV_Group_ID} A certain group (x, y, z) Indicated by @<code>{SV_GroupThreadID} The threads contained in a group (x, y, z) Indicate.


==== Specific calculation example (1)


For example, suppose you want to run a kernel that runs in (2, 2, 1) groups with (4, 1, 1) threads.
One of them runs in the (2, 0, 0)th thread of the (0, 1, 0)th group.
At this time @<code>{SV_DispatchThreadID} Is (0, 1, 0) * (4, 1, 1) + (2, 0, 0) = (0, 1, 0) + (2, 0, 0) = (2, 1, 0) Will be.


==== Specific calculation example (2)


Now let's consider the maximum value. In the (2, 2, 1) group, when the kernel runs with (4, 1, 1) threads,
(1, 1, 0) In the second group, (3, 0, 0) The second thread is the last thread.
At this time @<code>{SV_DispatchThreadID} Is (1, 1, 0) * (4, 1, 1) + (3, 0, 0) = (4, 1, 0) + (3, 0, 0) = (7, 1, 0) Will be.


=== Write value to texture (pixel)


Since it is difficult to explain in chronological order, check the entire sample while checking.

sample (2) of @<code>{dispatchThreadID.xy} Is
I've set up a group and a thread to show all the pixels on the texture.
Since it is the script side that sets up the group, we need to see it across the script and the compute shader.

//emlist[SimpleComputeShader_Texture.compute]{
textureBuffer[dispatchThreadID.xy]
    = float4(dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             1);
//}

In this sample 512x512 Although the texture of is prepared, @<code>{dispatchThreadID.x} But 0 ~ 511 When
@<code>{dispatchThreadID / width} Is 0 ~ 0.998… Indicates.
That is @<code>{dispatchThreadID.xy} As the value of increases (= pixel coordinates), it will fill from black to white.

//note{
The texture consists of RGBA channels and is set between 0 and 1.
When all 0s, it becomes completely black, when all 1s, it becomes completely white.
//}


=== Prepare the texture


Below is the explanation of the implementation on the script side. In sample (1), an array buffer is prepared to store the calculation result of the compute shader.
For sample (2), prepare a texture instead.

//emlist[SimpleComputeShader_Texture.cs]{
RenderTexture renderTexture_A;
…
void Start()
{
    this.renderTexture_A = new RenderTexture
        (512, 512, 0, RenderTextureFormat.ARGB32);
    this.renderTexture_A.enableRandomWrite = true;
    this.renderTexture_A.Create();
…
//}

Initialize RenderTexture with resolution and format.
At this time @<code>{RenderTexture.enableRandomWrite} Enable
Note that we have enabled writing to the texture.

 * RenderTexture.enableRandomWrite - Unity
 ** @<href>{https://docs.unity3d.com/ScriptReference/RenderTexture-enableRandomWrite.html}


=== Get the number of threads


You can also get how many threads the kernel can run (thread size) just as you can get the index of the kernel.

//emlist[SimpleComputeShader_Texture.cs]{
void Start()
{
…
    uint threadSizeX, threadSizeY, threadSizeZ;

    this.computeShader.GetKernelThreadGroupSizes
     (this.kernelIndex_KernelFunction_A,
      out threadSizeX, out threadSizeY, out threadSizeZ);
…
//}


=== Kernel execution


@<code>{Dispath} The process is executed by the method. At this time, pay attention to the method of specifying the number of groups.
In this example, the number of groups is calculated as "resolution of texture in horizontal (vertical) direction / number of threads in horizontal (vertical) direction".

When considering the horizontal direction, the texture resolution is 512 and the number of threads is 8, so
The number of groups in the horizontal direction is 512/8 = 64. Similarly, the vertical direction is 64.
Therefore, the total number of groups is 64 * 64 = 4096.

//emlist[SimpleComputeShader_Texture.cs]{
void Update()
{
    this.computeShader.Dispatch
    (this.kernelIndex_KernelFunction_A,
     this.renderTexture_A.width  / this.kernelThreadSize_KernelFunction_A.x,
     this.renderTexture_A.height / this.kernelThreadSize_KernelFunction_A.y,
     this.kernelThreadSize_KernelFunction_A.z);

    plane_A.GetComponent<Renderer>()
        .material.mainTexture = this.renderTexture_A;
//}

In other words, each group will process 8 * 8 * 1 = 64 (= number of threads) pixels.
Since there are 4096 groups, we will process 4096 * 64 = 262,144 pixels.
The image is 512 * 512 = 262,144 pixels, which means that all pixels could be processed in parallel.

==== Running different kernels


The other kernel fills using the y coordinate instead of x.
Note that at this time, a value close to 0, a black color appears at the bottom.
You may need to consider the origin when working with textures.


=== Advantages of multidimensional threads, groups


Multidimensional threads and groups work well when you need multidimensional results or multidimensional operations, like in sample (2).
If you try to process sample (2) in a one-dimensional thread, you need to compute the vertical pixel coordinates arbitrarily.

//note{
You can confirm it by actually implementing it, but when there is a stride in the image processing, for example, a 512x512 image,
The 513th pixel needs to be calculated as (0, 1) coordinates.
//}

It is better to reduce the number of operations, and the complexity increases with advanced processing.
When designing a process that uses a compute shader, it is a good idea to consider whether you can utilize multidimensionality well.


== Additional information for further learning

In this chapter, we used introductory information in the form of explaining a sample about the compute shader.
In the future, I will supplement some information that will be necessary for further learning.

=== GPU Architecture/basic structure


//image[primerofcomputeshader02][GPU Image of architecture][scale=1]

If you have a basic knowledge of GPU architecture and structure,
I will introduce it here a little because it helps to optimize it when implementing processing using Compute Shader.

GPU Are numerous @<b>{Streaming Multiprocessor(SM)} Is installed,
They share and parallelize and execute the given process.

SM is even smaller @<b>{Streaming Processor(SP)} Is installed,
SM The process assigned to SP Is calculated by.

SM has @<b>{registers} and @<b>{shared memory},
You can read and write faster than @<b>{Global memory (memory on DRAM)}.
Registers are used for local variables that are only referenced inside functions,
The shared memory can be referenced and written by all SPs that belong to the same SM.

In other words, grasp the maximum size and scope of each memory,
Ideally, you should be able to implement the optimum implementation that can read and write memory without waste and at high speed.

For example, the shared memory that you need to consider most is
Storage-class modifiers Defined using @<code>{groupshared}.
Since this section is an introduction, I will omit specific examples of introduction, but please remember them as techniques and terms necessary for optimization, and use them for subsequent learning.

 * Variable Syntax - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/bb509706(v=vs.85).aspx}


==== register


It is the memory area that is located closest to the SP and has the fastest access.
It consists of 4 bytes, and kernel (function) scope variables are allocated.
Since each thread is independent, it cannot be shared.


==== Shared memory


A memory area that resides on the SM and is managed along with the L1 cache.
It can be shared by SPs (= threads) in the same SM, and can be accessed fast enough.


==== Global memory


A memory area on DRAM, not on the GPU.
The reference is slow because it is far from the processor on the GPU.
On the other hand, it has a large capacity and data can be read and written from all threads.


==== Local memory


The memory area on DRAM, not on the GPU, stores data that does not fit in the registers.
The reference is slow because it is far from the processor on the GPU.


==== Texture memory


This is a memory dedicated to texture data, and the global memory is used only for textures.


==== Constant memory


This is a read-only memory and is used to store the arguments and constants of the kernel (function).
It has its own cache and can be referenced faster than global memory.


=== Tips for efficiently specifying the number of threads

If the total number of threads is larger than the number of data you actually want to process,
This is inefficient because it results in threads that are meaninglessly executed (or not processed).
Design the total number of threads to match the number of data you want to process as much as possible.

=== Limits on current specifications


I will introduce the upper limit of the current specifications at the time of writing. Please be aware that it may not be the latest version.
However, it is required to implement it while considering such restrictions.

 * Compute Shader Overview - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/ff476331(v=vs.85).aspx}


==== Number of threads and groups


I did not mention the limit of the number of threads or groups in the explanation.
This is because it changes depending on the shader model (version).
It seems that the number that can be paralleled will increase in the future.

 * ShaderModel cs_4_x
 ** Z The maximum value of 1
 ** X * Y * Z The maximum value of 768

 * ShaderModel cs_5_0
 ** Z The maximum value of 64
 ** X * Y * Z The maximum value of 1024

Also, the group limit is (x, y, z) And 65535 respectively.


==== Memory area


The upper limit of shared memory is per unit group 16 KB, 
The size of shared memory that a thread can write is limited to 256 bytes per unit.


== reference


Other references in this chapter are:

 * #5 GPU structure-Japan GPU Computing Partnership-@<href>{http://www.gdep.jp/page/view/252}
 * Windows Start with CUDA getting Started - Nvidia Japan - @<href>{http://on-demand.gputechconf.com/gtc/2013/jp/sessions/8001.pdf}