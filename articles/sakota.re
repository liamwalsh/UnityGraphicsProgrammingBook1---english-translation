
= Fluid simulation by grid method

== About this chapter

In this chapter, we will explain the fluid simulation by the grid method using Compute Shader.



== Sample data

=== code
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming/,https://github.com/IndieVisualLab/UnityGraphicsProgramming/}  

のAssets/StabeFluidsに格納されています。

=== Execution environment
 * Shader model 5.0 compatible environment that ComputeShader can execute
 * Environment confirmed at the time of writing, Unity5.6.2, Unity2017.1.1

== Introduction

In this chapter, we will explain the fluid simulation by the grid method and the calculation method and the way of understanding the mathematical formulas that are necessary to realize them. First of all, what is the lattice method? In order to find out its meaning, let's take a closer look at the method of analyzing “flow” in hydrodynamics.

=== How to think in fluid mechanics

Fluid mechanics is characterized by formulating a natural phenomenon, "flow", and making it computable. How can this "flow" be digitized and analyzed? @<br>{}
If you go straight, it can be quantified by guiding "the flow velocity when the time advances momentarily". Mathematically speaking, it can be translated into the analysis of the amount of change in the velocity vector when differentiated with respect to time. @<br>{}
However, there are two possible methods to analyze this flow. @<br>{}
One is to divide the hot water in the bath into grids and measure the flow velocity vector of each fixed grid space when you imagine the hot water in the bath. @<br>{}
And the other is to float the duck in the bath and analyze the duck movement itself. Of these two methods, the former is called "Euler's method" and the latter is called "Lagrange's method".

=== Various fluid simulations

Now let's get back to computer graphics. There are several simulation methods for fluid simulation, such as "Euler's method" and "Lagrange's method", but they can be broadly divided into the following three types.

 * Lattice method (e.g. Stable Fluid)
 * Particle method (e.g. SPH)
 * Lattice method + particle method (e.g. FLIP)

It may be a little imaginable from the meaning of the kanji, but the grid method, like the Euler method, creates a grid-shaped "field" when simulating a flow, and differentiates it by time. It is a method of simulating the speed of each grid.
The particle method is a method of simulating the advection of the particle itself, focusing on the particle, as in the "Lagrange method". @<br>{}
Along with the lattice method and the particle method, there are areas where we are good and bad at each other. @<br>{}
The lattice method is good at calculating pressure, viscosity, diffusion, etc. in fluid simulation, but not good at advection calculation. @<br>{}
On the contrary, the particle method is good at calculating advection. (These strengths and weaknesses can be imagined when you think of how to analyze Euler's method and Lagrange's method.)@<br>{}
In order to supplement these, methods such as the lattice method + particle method, which are typified by the FLIP method, have been born to complement each other's specialty fields.

In this paper, based on Jon Stam's Stable Fluids, which is a paper on incompressible viscous fluid simulation in the lattice method presented at SIGGRAPH 1999., we will explain the implementation method of fluid simulation and the mathematical formulas required for simulation. ..

== On the Navier-Stokes equation

First, let's look at the Navier-Stokes equation in the lattice method.

//texequation{
\dfrac {\partial \overrightarrow {u}} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u} + \nu \nabla ^{2} \overrightarrow {u} + \overrightarrow{f}
//}

//texequation{
\dfrac {\partial \rho} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \rho + \kappa \nabla ^{2} \rho + S
//}

//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}

Of the above, the first equation represents the velocity field and the second the density field. The third is "Continuity formula (mass conservation law)".
Let's try unraveling these three expressions one by one.


== Formula of continuity (mass conservation law)


First, let's unravel from the "continuous equation (mass conservation law)", which is a short equation and serves as a condition when simulating an "incompressible" fluid. @<br>{}
When simulating a fluid, you need to make a clear distinction between what is compressible and what is incompressible. For example, if the target is a substance whose density such as gas changes with pressure, it will be a compressible fluid. On the other hand, if the density of water is constant at any place, it is an incompressible fluid. @<br>{}
Since this chapter deals with the simulation of incompressible fluids, the divergence of each cell in the velocity field should be kept at zero. In other words, it cancels the inflow and outflow of the velocity field and keeps it at 0. If there is an inflow, it will flow out, so the flow velocity will propagate. This condition can be expressed by the following equation as a continuous equation (mass conservation law).



//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}



The above means that "divergence is 0". First, let's check the formula for "divergence".


=== Divergence


//texequation{
\nabla \cdot \overrightarrow{u} = \nabla \cdot (u, v) = \dfrac{\partial u}{\partial x} + \dfrac{\partial v}{\partial y}
//}



@<m>{\nabla}(Nabla operator) is called vector differential operator. For example, assuming that the vector field is two-dimensional,@<m>{ \left( \dfrac {\partial \} {\partial x\}_, \dfrac {\partial \} {\partial y\} \right) }の偏微分を取る際の、偏微分の表記を簡略化した演算子として作用します。@<m>{\nabla}演算子は演算子ですので、それだけでは意味を持ちませんが、一緒に組み合わせる式が内積なのか、外積なのか、それとも単に@<m>{\nabla f}といった関数なのかで演算内容が変わってきます。@<br>{}
This time, let's talk about "divergence," which is the inner product of partial derivatives. First, let's see why this expression means "divergence".



In order to understand the divergence, let's first consider cutting out one cell in the lattice space as shown below.



//image[divergence-s][Extract cells of differential interval (Δx, Δy) from vector field]{
//}




Divergence is the calculation of how many vectors flow into and out of one cell in the vector field. Outflow is + and inflow is -.

The divergence is the amount of change between a specific point x in the x direction and a slight amount of @<m>{\Delta x} when looking at the partial derivative when the cell of the vector field is cut off as described above, or , It can be calculated by the inner product of the change amount of the specific point y in the y direction and the slightly advanced @<m>{\Delta y}.
The reason why the outflow is obtained by the inner product with the partial derivative can be proved by differentiating the above figure.


//texequation{
\frac{i(x + \Delta x, y)\Delta y - i(x,y)\Delta y + j(x, y + \Delta y)\Delta x - j(x,y)\Delta x }{\Delta x \Delta y}
//}

//texequation{
 = \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \frac{j(x, y+\Delta y) - j(x,y)}{\Delta y}
//}



上記の式から極限をとり、



//texequation{
\lim_{\Delta x \to 0} \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \lim_{\Delta y \to 0} \frac{j(x,y+\Delta y) - j(x,y)}{\Delta y} = \dfrac {\partial i} {\partial x} + \dfrac {\partial j} {\partial y}
//}



By doing, you can finally find the equation and the equation of the inner product with the partial derivative.


== Velocity field


Next, I will explain the velocity field, which is the main point of the lattice method.
Before that, let's confirm the gradient and Laplacian in addition to the divergence confirmed earlier when implementing the Navier-Stokes equation of the velocity field.


=== Gradient


//texequation{
\nabla f(x, y) = \left( \dfrac{\partial f}{\partial x}_,\dfrac{\partial f}{\partial y}\right)
//}


@<m>{\nabla f (grad \ f)}Is the formula for the gradient. The meaning is that by sampling the coordinates slightly advanced in each partial differential direction with the function @<m>{f} and synthesizing the obtained values ​​in each partial differential direction, which vector is finally determined. It means facing. In other words, it is possible to calculate the vector that is oriented in the direction with the larger value when the partial differentiation is performed.


=== Laplacian


//texequation{
\Delta f = \nabla^2 f = \nabla \cdot \nabla f = \frac{\partial^2 f}{\partial x^2} + \frac{\partial^2 f}{\partial y^2}
//}



Laplacian is represented by the symbol of Nabla inverted upside down. (Same as Delta, but read from the context, don't make a mistake.)@<br>{}
@<m>{\nabla^2 f}Or@<m>{\nabla \cdot \nabla f}It is also written as and is calculated as the second partial derivative.@<br>{}
Also, when it is disassembled and considered, it can be taken as a form in which the gradient of the function is taken and the divergence is obtained.@<br>{}
In terms of meaning, there are many inflows at the points concentrated in the gradient direction in the vector field, so there is a divergence. I can imagine that.@<br>{}
There are two types of Laplacian operators, scalar Laplacian and vector Laplacian. When acting on a vector field, gradient, divergence, and rotation (the outer product of ∇ and the vector) are used.@<br>{}
//texequation{
\nabla^2 \overrightarrow{u} = \nabla \nabla \cdot \overrightarrow{u} - \nabla \times \nabla \times \overrightarrow{u}
//}
However, only in the case of Cartesian coordinate system, the gradient and divergence can be obtained for each vector component, and can be obtained by combining them.



//texequation{
\nabla^2 \overrightarrow{u} = \left(
\dfrac{\partial ^2 u_x}{\partial x^2}+\dfrac{\partial ^2 u_x}{\partial y^2}+\dfrac{\partial ^2 u_x}{\partial z^2}_,
\dfrac{\partial ^2 u_y}{\partial x^2}+\dfrac{\partial ^2 u_y}{\partial y^2}+\dfrac{\partial ^2 u_y}{\partial z^2}_,
\dfrac{\partial ^2 u_z}{\partial x^2}+\dfrac{\partial ^2 u_z}{\partial y^2}+\dfrac{\partial ^2 u_z}{\partial z^2}
\right)
//}



This completes the confirmation of the mathematical formulas required to solve the Navier-Stokes equations in the lattice method.
From here, let's look at the velocity field equation for each term.


=== Confirmation of velocity field from Navier-Stokes equation


//texequation{
\dfrac {\partial \overrightarrow {u}} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u} + \nu \nabla ^{2} \overrightarrow {u} + \overrightarrow {f}
//}



Of the above,@<m>{\overrightarrow {u\}}Is the flow velocity,@<m>{\nu}Is the kinematic viscosity coefficient（kinematic viscosity）、@<m>{\overrightarrow{f\}}Is an external force.@<br>{}
You can see that the left side is the flow velocity when partial differentiation is taken with time. On the right side, the first term is the advection term, the second term is the diffusion viscosity term, the third term is the pressure term, and the fourth term is the external force term.



Even if these can be done in a batch at the time of calculation, it is necessary to implement them in steps when implementing them. @<br>{}
First of all, as a step, if you do not receive an external force, you cannot make changes under the initial conditions, so I would like to consider from the external force term of the fourth term.


=== Force item outside the velocity field


This is the part that simply adds the vector from the outside. In other words, when the velocity field is 0 in the initial condition, the vector is added to the corresponding ID of RWTexture2D from the UI or some event as the starting point of the vector. @<br>{}
The kernel of the external force term of compute shader is implemented as follows. Also, describe the definition of each coefficient and buffer that will be used in the compute shader.


//emlist{
float visc;                   //Kinematic viscosity coefficient
float dt;                     //Delta time
float velocityCoef;           //Velocity external force coefficient
float densityCoef;            //Pressure coefficient outside density field

//xy = velocity, z = density, Fluid solver to pass to the drawing shader
RWTexture2D<float4> solver;
//density field, Density field
RWTexture2D<float>  density;  
//velocity field, Velocity field
RWTexture2D<float2> velocity; 
//xy = pre vel, z = pre dens. when project, x = p, y = div
//Save buffer one step before and temporary buffer when saving mass
RWTexture2D<float3> prev;
//xy = velocity source, z = density source 外力入力バッファ
Texture2D source;             

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AddSourceVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        velocity[id] += source[id].xy * velocityCoef * dt;
        prev[id] = float3(source[id].xy * velocityCoef * dt, prev[id].z);
    }
}
//}


The next step is to implement the second term, the diffusive viscosity term.


=== Velocity field


//texequation{
\nu \nabla ^{2} \overrightarrow {u}
//}



@<m>{\nabla}演算子や@<m>{\Delta}When there is a value on the left and right of the operator, there is a rule that "acts only on the right element", so in this case, leave the kinematic viscosity coefficient once and consider the vector Laplacian part first. @<br>{}
The vector Laplacian is used for the flow velocity @<m>{\overrightarrow{u\}} to synthesize the gradient and divergence of each component of the vector, and diffuse the flow velocity to the adjacent. By multiplying it by the kinematic viscosity coefficient, the diffusion momentum is adjusted. @<br>{}
Here, since the gradient of each component of the flow velocity is taken and then diffused, it may be possible to understand the phenomenon that inflow from and outflow from adjacent neighbors occur, and the vector received in step 1 affects adjacent neighbors. think. @<br>{}
On the mounting side, some ingenuity is required. If implemented according to the formula, vibration will occur when the diffusivity obtained by multiplying the viscosity coefficient by the differential time and the number of grids becomes high, and convergence will not be achieved and the simulation itself will eventually diverge. @<br>{}
In order to make the diffusion stable, the iterative methods such as Gauss-Seidel method, Jacobi method and SOR method are used here. Here, let's simulate the Gauss-Seidel method. @<br>{}
The Gauss-Seidel method is a method in which an equation is converted into a linear equation consisting of unknowns for its own cell, the calculated value is immediately used in the next iteration, and a chain is made to converge to an approximate answer. More iterations will converge to more accurate values, but the number of iterations is machine Adjust it considering performance and appearance.


//emlist{
#define GS_ITERATE 4

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void DiffuseVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float a = dt * visc * w * h;

        [unroll]
        for (int k = 0; k < GS_ITERATE; k++) {
            velocity[id] = (prev[id].xy + a * (
                            velocity[int2(id.x - 1, id.y)] +
                            velocity[int2(id.x + 1, id.y)] +
                            velocity[int2(id.x, id.y - 1)] +
                            velocity[int2(id.x, id.y + 1)]
                            )) / (1 + 4 * a);
            SetBoundaryVelocity(id, w, h);
        }
    }
}
//}


上記のSetBoundaryVelocity関数は境界用のメソッドになります。詳しくはリポジトリをご参照下さい。


=== Quality preservation


//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}



Now let's go back to the mass storage side before proceeding with the section. In the process up to this point, the force received by the external force term was diffused into the velocity field, but at present, the mass of each cell is not preserved, and the mass of the place where there is a lot of inflow and the place where there are many inflows Is not saved. @<br>{}
As in the above equation, the mass must be saved and the divergence of each cell must be set to 0, so save the mass here. @<br>{}
In addition, when performing the mass preservation step with ComputeShader, the field must be fixed because it performs a partial differential operation with the adjacent thread.
If partial differential operation could be performed in the group shared memory, speedup could be expected, but when partial differential was taken from another group thread, the value could not be obtained and the result was dirty, so buffer here. While confirming, proceed in 3 steps. @<br>{}
Calculate divergence from velocity field> Calculate Poisson equation by Gauss-Seidel method> Subtract velocity field and save mass@<br>{}
Divide the kernel into the three steps of, and bring it to the mass conservation while establishing the field. The SetBound~ system is a method call for the boundary.


//emlist{
//Quality preservation Step1.
//In step 1, calculate the divergence from the velocity field
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep1(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        prev[id] = float3(0.0,
                    -0.5 * 
                    (uvd.x * (velocity[int2(id.x + 1, id.y)].x -
                              velocity[int2(id.x - 1, id.y)].x)) +
                    (uvd.y * (velocity[int2(id.x, id.y + 1)].y - 
                              velocity[int2(id.x, id.y - 1)].y)),
                    prev[id].z);

        SetBoundaryDivergence(id, w, h);
        SetBoundaryDivPositive(id, w, h);
    }
}

//Quality preservation Step 2.
//In step2, the Poisson equation is solved by the Gauss-Seidel method from the divergence obtained in step1.
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep2(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        for (int k = 0; k < GS_ITERATE; k++)
        {
            prev[id] = float3(
                        (prev[id].y + prev[uint2(id.x - 1, id.y)].x + 
                                      prev[uint2(id.x + 1, id.y)].x +
                                      prev[uint2(id.x, id.y - 1)].x +
                                      prev[uint2(id.x, id.y + 1)].x) / 4,
                        prev[id].yz);
            SetBoundaryDivPositive(id, w, h);
        }
    }
}

//Quality preservation Step 3.
//step3 so、∇･u = 0 To
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep3(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float  velX, velY;
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        velX = velocity[id].x;
        velY = velocity[id].y;

        velX -= 0.5 * (prev[uint2(id.x + 1, id.y)].x - 
                       prev[uint2(id.x - 1, id.y)].x) / uvd.x;
        velY -= 0.5 * (prev[uint2(id.x, id.y + 1)].x -
                       prev[uint2(id.x, id.y - 1)].x) / uvd.y;

        velocity[id] = float2(velX, velY);
        SetBoundaryVelocity(id, w, h);
    }
}
//}


With this, the velocity field is in the state of mass conservation. Inflow occurs at the place where it flows out, and outflow occurs at the place where there is a large amount of inflow.


=== Advection term


//texequation{
-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u}
//}



For the advection term, Lagrange's method is used, but it is necessary to perform backtracking of the velocity field one step before and move the value of the location where the velocity vector is subtracted from the relevant cell to the current location. Do this for each cell.
When backtracing, it does not go back to a place that fits exactly on the grid, so at the time of advection, linear interpolation with the neighboring 4 cells is performed and the correct value is advected.


//emlist{
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AdvectVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    density.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        int ddx0, ddx1, ddy0, ddy1;
        float x, y, s0, t0, s1, t1, dfdt;

        dfdt = dt * (w + h) * 0.5;

        //Backtrace point calculation.
        x = (float)id.x - dfdt * prev[id].x;
        y = (float)id.y - dfdt * prev[id].y;
        //Clamp the points so that they are within the simulation range.
        clamp(x, 0.5, w + 0.5);
        clamp(y, 0.5, h + 0.5);
        //Near cell index of back trace point.
        ddx0 = floor(x);
        ddx1 = ddx0 + 1;
        ddy0 = floor(y);
        ddy1 = ddy0 + 1;
        //Save the difference for linear interpolation with neighboring cells.
        s1 = x - ddx0;
        s0 = 1.0 - s1;
        t1 = y - ddy0;
        t0 = 1.0 - t1;

        //Back trace, take the value of 1 step before with linear interpolation and substitute it into the current velocity field.
        velocity[id] = s0 * (t0 * prev[int2(ddx0, ddy0)].xy +
                             t1 * prev[int2(ddx0, ddy1)].xy) +
                       s1 * (t0 * prev[int2(ddx1, ddy0)].xy +
                             t1 * prev[int2(ddx1, ddy1)].xy);
        SetBoundaryVelocity(id, w, h);
    }
}
//}

== Density field


Next, let's see the density field equation.



//texequation{
\dfrac {\partial \rho} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \rho + \kappa \nabla ^{2} \rho + S
//}



上記の内、@<m>{\overrightarrow {u\}}Is the flow velocity,@<m>{\kappa}Is the diffusion coefficient, ρ is the density, and S is the external pressure.@<br>{}
The density field is not always necessary, but by adding the pixels on the screen diffused by the density field to each vector when the velocity field is obtained, it becomes possible to express a more fluid-like fluid like melting. I will. @<br>{}
As you may have noticed by looking at the density field formula, the flow is exactly the same as the velocity field, the difference is that the vector is a scalar and the kinematic viscosity coefficient @<m>{ There are only three points: the point where \nu} is the diffusion coefficient @<m>{\kappa} and the point where the law of conservation of mass is not used. @<br>{}
Since the density field is the field of change in density, it does not need to be incompressible and does not require mass conservation. The kinematic viscosity coefficient and the diffusion coefficient have the same usage as coefficients. @<br>{}
Therefore, it is possible to implement the density field by reducing the dimension of the kernel other than the mass conservation law of the kernel used in the velocity field.
I will not explain the density field on paper, but please refer to that as well because the density field is implemented in the repository.

== Simulation term steps


A fluid can be simulated by using the above velocity field, density field, and mass conservation law, but let's look at the simulation steps at the end.

 * Generate an external force event and input it into the external force terms of the velocity and density fields
 * Update the velocity field in the following steps
 ** Loose sticky term
 ** Quality preservation
 ** Advection term
 ** Quality preservation
 * Then update the density field in the following steps
 ** Loose item
 ** Advancing density using velocity in velocity field

The above is the simulation step of StableFluid.


== result

By executing and dragging on the screen with the mouse, the following fluid simulation can be triggered.

//image[fluid-s][Execution example][scale=0.7]{
//}

== Summary

Unlike pre-rendering, fluid simulation is a heavy field for real-time game engines like Unity.
However, due to improvements in GPU computing power, it has become possible to produce FPS that can withstand a certain level of resolution in two dimensions.
In addition, if you try to implement the Gauss-Seidel iterative method, which is a heavy load on the GPU that came out on the way, with another process, or substitute the curl noise for the velocity field itself, It will be possible to express fluids with lighter calculations.

If you have read this chapter and are interested in fluids, please try the next chapter, "Fluid Simulation by Particle Method".
Since you can approach the fluid from a different angle from the grid method, I think that you can experience the depth of fluid simulation and the fun of mounting.

== reference

 * Jos Stam. SIGGRAPH 1999. Stable Fluids
