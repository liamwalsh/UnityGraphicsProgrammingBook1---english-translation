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


@<code>{groupThreadID(SV_GroupThreadID)} は、
今このカーネルが、グループ内の何番目のスレッドで実行されているかを示す値が入るので、
この例では (0 ~ 3, 0, 0) が入ります。したがって、@<code>{groupThreadID.x} は 0 ~ 3 です。
つまり、@<code>{intBuffer[0] = 0}　～ @<code>{intBuffer[3] = 3} までが並列して実行されることになります。


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
It's a little complicated,@<b>{「あるカーネルを実行するスレッドが、すべてのスレッドの中のどこに位置するか (x,y,z) 」}Is shown.

@<code>{SV_DispathThreadID} は、@<code>{SV_Group_ID * numthreads + SV_GroupThreadID} で算出される値です。
@<code>{SV_Group_ID} はあるグループを (x, y, z) で示し、@<code>{SV_GroupThreadID} は、あるグループに含まれるスレッドを (x, y, z) で示します。


==== 具体的な計算例 (1)


例えば、(2, 2, 1) グループで、(4, 1, 1) スレッドで実行される、カーネルを実行するとします。
その内の 1 つのカーネルは、(0, 1, 0) 番目のグループに含まれる、(2, 0, 0) 番目のスレッドで実行されます。
このとき @<code>{SV_DispatchThreadID} は、(0, 1, 0) * (4, 1, 1) + (2, 0, 0) = (0, 1, 0) + (2, 0, 0) = (2, 1, 0) になります。


==== 具体的な計算例 (2)


今度は最大値を考えましょう。(2, 2, 1) グループで、(4, 1, 1) スレッドでカーネルが実行されるとき、
(1, 1, 0) 番目のグループに含まれる、(3, 0, 0) 番目のスレッドが最後のスレッドです。
このとき @<code>{SV_DispatchThreadID} は、(1, 1, 0) * (4, 1, 1) + (3, 0, 0) = (4, 1, 0) + (3, 0, 0) = (7, 1, 0) になります。


=== テクスチャ (ピクセル) に値を書き込む


以降は時系列順に解説するのが困難ですので、サンプル全体に目を通しながら確認してください。

サンプル (2) の @<code>{dispatchThreadID.xy} は、
テクスチャ上にあるすべてのピクセルを示すように、グループとスレッドを設定しています。
グループを設定するのはスクリプト側なので、スクリプトとコンピュートシェーダを横断して確認する必要があります。

//emlist[SimpleComputeShader_Texture.compute]{
textureBuffer[dispatchThreadID.xy]
    = float4(dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             1);
//}

このサンプルでは仮に 512x512 のテクスチャを用意していますが、@<code>{dispatchThreadID.x} が 0 ~ 511 を示すとき、
@<code>{dispatchThreadID / width} は、0 ~ 0.998… を示します。
つまり @<code>{dispatchThreadID.xy} の値( = ピクセル座標)が大きくなるにつれて、黒から白に塗りつぶしていくことになります。

//note{
テクスチャは、RGBA チャネルから構成され、それぞれ 0 ~ 1 で設定します。
すべて 0 のとき、完全に黒くなり、すべて 1 のとき、完全に白くなります。
//}


=== テクスチャの用意


以降がスクリプト側の実装の解説です。サンプル (1) では、コンピュートシェーダの計算結果を保存するために配列のバッファを用意しました。
サンプル (2) では、その代わりにテクスチャを用意します。

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

解像度とフォーマットを指定して RenderTexture を初期化します。
このとき @<code>{RenderTexture.enableRandomWrite} を有効にして、
テクスチャへの書き込みを有効にしている点に注意します。

 * RenderTexture.enableRandomWrite - Unity
 ** @<href>{https://docs.unity3d.com/ScriptReference/RenderTexture-enableRandomWrite.html}


=== スレッド数の取得


カーネルのインデックスが取得できるように、カーネルがどれくらいのスレッド数で実行できるかも取得できます(スレッドサイズ)。

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


=== カーネルの実行


@<code>{Dispath} メソッドで処理を実行します。このとき、グループ数の指定方法に注意します。
この例では、グループ数は「テクスチャの水平(垂直)方向の解像度 / 水平(垂直)方向のスレッド数」で算出しています。

水平方向について考えるとき、テクスチャの解像度は 512、スレッド数は 8 ですから、
水平方向のグループ数は 512 / 8 = 64 になります。同様に垂直方向も 64 です。
したがって、合計グループ数は 64 * 64 = 4096 になります。

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

言い換えれば、各グループは 8 * 8 * 1 = 64 (= スレッド数) ピクセルずつ処理することになります。
グループは 4096 あるので、4096 * 64 = 262,144 ピクセル処理します。
画像は、512 * 512 = 262,144 ピクセルなので、ちょうどすべてのピクセルを並列に処理できたことになります。


==== 異なるカーネルの実行


もう一方のカーネルは、x ではなく、 y 座標を使って塗りつぶしていきます。
このとき 0 に近い値、黒い色が下のほうに表れている点に注意します。
テクスチャを操作するときは原点を考慮しなければならないこともあります。


=== 多次元スレッド、グループの利点


サンプル (2) のように、多次元の結果が必要な場合、あるいは多次元の演算が必要な場合には、多次元のスレッドやグループが有効に働きます。
もしサンプル (2) を 1 次元のスレッドで処理しようとすると、縦方向のピクセル座標を任意に算出する必要があります。

//note{
実際に実装しようとすると確認できますが、画像処理でいうところのストライド、例えば 512x512 の画像があるとき、
その 513 番目のピクセルは、(0, 1) 座標になる、といった算出が必要になります。
//}

演算数は削減したほうが良いですし、高度な処理を行うにしたがって複雑さは増します。
コンピュートシェーダを使った処理を設計するときは、上手く多次元を活用できないか検討するのが良いです。


== さらなる学習のための補足情報


本章ではコンピュートシェーダについてサンプルを解説する形式で入門情報としましたが、
これから先、学習を進める上で必要ないくつかの情報を補足します。


=== GPU アーキテクチャ・基本構造


//image[primerofcomputeshader02][GPU アーキテクチャのイメージ][scale=1]

GPU のアーキテクチャ・構造についての基本的な知識があれば、
コンピュートシェーダを使った処理の実装の際、それを最適化するために役に立つので、少しだけここで紹介します。

GPU は多数の @<b>{Streaming Multiprocessor(SM)} が搭載されていて、そ
れらが分担・並列化して与えられた処理を実行します。

SM には更に小さな @<b>{Streaming Processor(SP)} が複数搭載されていて、
SM に割り当てられた処理を SP が計算する、といった形式です。

SM には@<b>{レジスタ}と@<b>{シェアードメモリ}が搭載されていて、
@<b>{グローバルメモリ(DRAM上のメモリ)}よりも高速に読み書きすることができます。
レジスタは関数内でのみ参照されるローカル変数に使われ、
シェアードメモリは同一 SM 内に所属するすべての SP から参照し書き込むことができます。

つまり、各メモリの最大サイズやスコープを把握し、
無駄なく高速にメモリを読み書きできる最適な実装を実現できるのが理想です。

例えば最も考慮する必要があるであろうシェアードメモリは、
クラス修飾子 (storage-class modifiers) @<code>{groupshared} を使って定義します。
ここでは入門なので具体的な導入例を割愛しますが、最適化に必要な技術・用語として覚えておいて、以降の学習に役立ててください。

 * Variable Syntax - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/bb509706(v=vs.85).aspx}


==== レジスタ


SP に最も近い位置に置かれ、最も高速にアクセスできるメモリ領域です。
4 byte 単位で構成され、カーネル(関数)スコープの変数が配置されます。
スレッドごとに独立するため共有することができません。


==== シェアードメモリ


SM に置かれるメモリ領域で、L1 キャッシュと合わせて管理されています。
同じ SM 内にある SP(= スレッド) で共有することができ、かつ十分に高速にアクセスすることができます。


==== グローバルメモリ


GPU 上ではなく DRAM 上のメモリ領域です。
GPU 上にのプロセッサからは離れた位置にあるため参照は低速です。
一方で、容量が大きく、すべてのスレッドからデータの読み書きが可能です。


==== ローカルメモリ


GPU 上ではなく DRAM 上のメモリ領域で、レジスタに収まらないデータを格納します。
GPU 上のプロセッサからは離れた位置にあるため参照は低速です。


==== テクスチャメモリ


テクスチャデータ専用のメモリで、グローバルメモリをテクスチャ専用に扱います。


==== コンスタントメモリ


読み込み専用のメモリで、カーネル(関数)の引数や定数を保存しておくためなどに使われます。
専用のキャッシュを持っていて、グローバルメモリよりも高速に参照できます。


=== 効率の良いスレッド数指定のヒント


総スレッド数が実際に処理したいデータ数よりも大きい場合は、
無意味に実行される (あるいは処理されない) スレッドが生じることになり非効率です。
総スレッド数は可能な限り処理したいデータ数と一致させるように設計します。


=== 現行スペック上の限界


執筆時時点での現行スペックの上限を紹介します。最新版でない可能性があることに十分に注意してください。
ただし、これらのような制限を考慮しつつ実装することが求められます。

 * Compute Shader Overview - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/ff476331(v=vs.85).aspx}


==== スレッドとグループ数


スレッド数やグループ数の限界については、解説中に言及しませんでした。
これはシェーダモデル(バージョン)によって変更されるためです。
今後も並列できる数は増えていくものと思われます。

 * ShaderModel cs_4_x
 ** Z の最大値が 1
 ** X * Y * Z の最大値が 768

 * ShaderModel cs_5_0
 ** Z の最大値が 64
 ** X * Y * Z の最大値は 1024

またグループの限界は (x, y, z) でそれぞれ 65535 です。


==== メモリ領域


シェアードメモリの上限は、単位グループあたり 16 KB, 
あるスレッドが書き込めるシェアードメモリのサイズは、単位あたり 256 byte までと制限されています。


== 参考


本章でのその他の参考は以下の通りです。

 * 第５回　GPUの構造 - 日本GPUコンピューティングパートナーシップ - @<href>{http://www.gdep.jp/page/view/252}
 * Windows で始める CUDA 入門 - エヌビディアジャパン - @<href>{http://on-demand.gputechconf.com/gtc/2013/jp/sessions/8001.pdf}