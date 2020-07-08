
= Fluid simulation by SPH method

In the previous chapter, we explained how to create a fluid simulation using the grid method.
In this chapter, we will use another particle simulation method, the SPH method in particular, to represent the fluid motion.
Please be aware that some explanations may be inadequate, as the explanation is made with a little bit of clutter.

== Basic knowledge
=== Euler and Lagrangian perspectives
There are Euler's point of view and Lagrange's point of view as a method of observing the movement of a fluid.
The Euler's point of view is to fix the observation points at fixed intervals on the fluid @<b>{fixed} and analyze the movement of the fluid at the observation points.
On the other hand, the Lagrangian point of view is that the observation point that moves along the fluid flow is @<b>{floating} and the movement of the fluid at that observation point is observed (see @<img>{lagrange}). ..
Basically, the fluid simulation method using the Euler's viewpoint is called the grid method, and the fluid simulation method using the Lagrangian viewpoint is called the particle method.

//image[lagrange][Left: Euler-like, Right: Lagrange-like][scale=0.7]{
//}

=== Lagrange differentiation (material differentiation)
The Euler and Lagrangian viewpoints differ in the way they are calculated.
First, let us show the physical quantity @<fn>{quantity} expressed from the Euler point of view.
//footnote[quantity][Physical quantity refers to observable speed and mass. In short, you can think of it as having a unit.]
//texequation{
  \phi = \phi (\overrightarrow{x}, t)
//}
This is the time@<m>{t}Positioned at@<m>{\overrightarrow{x\}}Physical quantity@<m>{\phi}It means that · · ·
The time derivative of this physical quantity is
//texequation{
  \frac{\partial \phi}{\partial t}
//}
Can be expressed as
Of course, this is because the position of the physical quantity@<m>{\overrightarrow{x\}}Since it is fixed at, it is the derivative from the Euler's point of view.

//footnote[advect][The movement of the observation point along the flow is called advection.]
On the other hand, from the Lagrangian perspective, the observation point moves along the flow.@<fn>{advect}Therefore, the observation point itself is also a function of time.
Therefore, in the initial state@<m>{\overrightarrow{x\}_0}The observation point in@<m>{t}で
//texequation{
  \overrightarrow{x}(\overrightarrow{x}_0, t)
//}
Exists in Therefore, the notation of physical quantity
//texequation{
  \phi = \phi (\overrightarrow{x}(\overrightarrow{x}_0, t), t)
//}
Will be.
According to the definition of derivative,@<m>{\Delta t}Looking at the amount of change in physical quantity after a second
//texequation{
  \displaystyle \lim_{\Delta t \to 0} \frac{\phi(\overrightarrow{x}(\overrightarrow{x}_0, t + \Delta t), t + \Delta t) - \phi(\overrightarrow{x}(\overrightarrow{x}_0, t), t)}{\Delta t}
//}
//texequation{
  = \sum_i \frac{\partial \phi}{\partial x_i} \frac{\partial x_i}{\partial t} + \frac{\partial \phi}{\partial t}
//}
//texequation{
  = \left( \left( \begin{matrix}u_1\\u_2\\u_3\end{matrix} \right)
    \cdot
    \left( \begin{matrix} \frac{\partial}{\partial x_1}\\\frac{\partial}{\partial x_2}\\\frac{\partial}{\partial x_3} \end{matrix} \right)
    + \frac{\partial}{\partial t}
    \right) \phi\\
//}
//texequation{
  = (\frac{\partial}{\partial t} + \overrightarrow{u} \cdot {grad}) \phi
//}
Will be.
This is the time derivative of the physical quantity considering the movement of the observation point.
However, using this notation complicates the formula, so
//texequation{
  \dfrac{D}{Dt} := \frac{\partial}{\partial t} + \overrightarrow{u} \cdot {grad}
//}
It can be shortened by introducing the operator.
A series of operations that consider the movement of observation points is called Lagrange differentiation.
It may seem complicated at first glance, but in the particle method in which the observation points move, it is more convenient to express the formula from a Lagrangian perspective.

=== Fluid incompressible condition
A fluid can be considered to have no volume change if the velocity of the fluid is well below the speed of sound.
This is called the fluid incompressible condition and is expressed by the following formula.
//texequation{
  \nabla \cdot \overrightarrow{u} = 0
//}
This indicates that there is no upwelling or disappearance in the fluid.
Derivation of this formula involves a little complicated integration, so the explanation is omitted.@<fn>{bridson}To do.
Please keep in mind about "the fluid does not compress!"

//footnote[bridson]["Fluid Simulation for Computer Graphics - Robert Bridson" で詳しく解説されています。]

== Particle method simulation
Particle method, small fluid@<b>{粒子}Divide by and observe the fluid movement from a Lagrangian perspective.
These particles are the observation points in the previous section. Even though the “particle method” is used to describe a lot, many methods have been proposed at the present time, and as a famous one,

 * Smoothed Particle Hydrodynamics(SPH)law
 * Fluid Implicit Particle (FLIP) law
 * Particle In Cell (PIC) law
 * Moving Particle Semi-implicit (MPS) law
 * Material Point Method (MPM) law
And so on.

=== Derivation of Navier-Stokes equations in the particle method
First, the Navier-Stokes equation (hereinafter NS equation) in the particle method is described as follows.
//texequation{
  \dfrac{D \overrightarrow{u}}{Dt} = -\dfrac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
  \label{eq:navier}
//}
The shape is a little different from the NS equation that appeared in the lattice method in the previous chapter.
The advection term is completely gone, but if you look at the relationship between the Euler derivative and the Lagrange derivative, you can see that it can be transformed into this shape well.
In the particle method, the observation point is moved along the flow, so it is not necessary to consider the advection term when calculating the NS equation.
The advection calculation can be done by directly updating the particle position based on the acceleration calculated by the NS equation.

#@#Also, the NS equation is surprisingly just a modification of Newton's second law, @<m>{m\overrightarrow{a\} = \overrightarrow{f\}}.

Since a real fluid is a collection of molecules, it can be said that it is a kind of particle system.
However, it is impossible to calculate the actual number of molecules with a computer, so it is necessary to adjust it to a size that can be calculated.
@<img>{blob}Each grain shown in(@<fn>{blobfoot})Represents a portion of the fluid divided into calculable sizes.
Each of these grains has a mass@<m>{m}, Position vector@<m>{\overrightarrow{x\}}、
Speed ​​vector@<m>{\overrightarrow{u\}},volume@<m>{V}Can be thought of as having.
#@# Change notation of position vector of image to x
//image[blob][Particle approximation of fluid][scale=0.7]{
//}
External force on each of these grains@<m>{\overrightarrow{f\}}To calculate the equation of motion@<m>{m \overrightarrow{a\} = \overrightarrow{f\}}The acceleration is calculated by solving
You can decide how to move in the next time step.

//footnote[blobfoot][Called'Blob' in English]

As mentioned above, each particle moves by receiving some force from the surroundings. What is that "force"?
As a simple example, gravity@<m>{m \overrightarrow{g\}}Other than that, some other force should also be exerted by surrounding particles.
These forces are explained below.

==== pressure
The first force on a fluid particle is pressure.
The fluid always flows from higher pressure to lower pressure.
If the pressure is the same from all directions, the force will be canceled and the movement will stop, so consider the case where the pressure is not balanced.
As mentioned in the previous section, by taking the gradient of the pressure scalar field, it is possible to calculate the direction with the highest rate of pressure rise from the viewpoint of one's own particle position.粒子が力を受ける方向は、圧力の高い方から低い方ですので、マイナスを取って@<m>{-\nabla p}となります。
Also, since particles have a volume, the pressure applied to them is@<m>{-\nabla p}It is calculated by multiplying the particle volume by@<fn>{vol}。
Finally,@<m>{- V \nabla p}The result is derived.

//footnote[vol][The incompressible condition of a fluid makes it possible to express the integral of the pressure exerted on a particle simply by multiplying it by volume.]

==== Viscous force
The second force on fluid particles is viscous force.
A viscous (sticky) fluid is a fluid that is difficult to deform, such as honey and melted chocolate.
Applying the word viscous to the expression of the particle method,
@<b>{The particle velocity is easy to average the surrounding particle velocity.}It means that.
As mentioned in the previous chapter, the operation of taking the average of the surroundings can be done using Laplacian.

The degree of viscosity@<b>{Kinematic viscosity coefficient}@<m>{\mu}When expressed using@<m>{\mu \nabla \cdot \nabla \overrightarrow{u\}}Can be expressed as

==== Integration of pressure, viscous force, and external force
Equations of motion for these forces@<m>{m \overrightarrow{a\} = \overrightarrow{f\}}If you apply it to
//texequation{
  m \dfrac{D\overrightarrow{u}}{Dt} = - V \nabla p + V \mu \nabla \cdot \nabla \overrightarrow{u} + m\overrightarrow{g}
//}
here,@<m>{m}は@<m>{\rho V}Since it is(@<m>{V}Will be canceled)
//texequation{
  \rho \dfrac{D\overrightarrow{u}}{Dt} = - \nabla p + \mu \nabla \cdot \nabla \overrightarrow{u} + \rho \overrightarrow{g}
//}
Both sides@<m>{\rho}Divide by
//texequation{
  \dfrac{D\overrightarrow{u}}{Dt} = - \dfrac{1}{\rho}\nabla p + \dfrac{\mu}{\rho} \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
//}
Finally, the coefficient of the viscosity term@<m>{\dfrac{\mu\}{\rho\}}に@<m>{\nu}Introduced
//texequation{
  \dfrac{D\overrightarrow{u}}{Dt} = - \dfrac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
//}
Then, I was able to derive the NS equation mentioned at the beginning.

=== Representation of advection in the particle method.
In the particle method, the particles themselves represent the observation points of the fluid, so the calculation of the advection term is completed simply by moving the particle position.
In the actual calculation of time derivative, infinitely small time is used,
Infinite time cannot be expressed by computer calculation, so the time is small enough.@<m>{\Delta t}Is used to express the derivative.
This is called difference,@<m>{\Delta t}The smaller is, the more accurate the calculation can be made.

Introducing the difference expression for acceleration,
//texequation{
  \overrightarrow{a} = \dfrac{D\overrightarrow{u}}{Dt} \equiv \frac{\Delta \overrightarrow{u}}{\Delta t}
//}
Will be.
Therefore speed increment@<m>{\Delta \overrightarrow{u\}}Is
//texequation{
\Delta \overrightarrow{u} = \Delta t \overrightarrow{a}
//}
And for position increments as well,
//texequation{
  \overrightarrow{u} = \frac{\partial \overrightarrow{x}}{\partial t} \equiv \frac{\Delta \overrightarrow{x}}{\Delta t}
//}
Than,
//texequation{
\Delta \overrightarrow{x} = \Delta t \overrightarrow{u}
//}
Will be.

You can use this result to calculate the velocity and position vectors for the next frame.
The particle velocity at the current frame@<m>{\overrightarrow{u\}_n}, Then
The particle velocity in the next frame is@<m>{\overrightarrow{u\}_{n+1\}}so,
//texequation{
\overrightarrow{u}_{n+1} = \overrightarrow{u}_n + \Delta \overrightarrow{u} = \overrightarrow{u}_n + \Delta t \overrightarrow{a}
//}
Can be expressed as

The particle position in the current frame@<m>{\overrightarrow{x\}_n}, Then
The particle position in the next frame is@<m>{\overrightarrow{x\}_{n+1\}}so,
//texequation{
\overrightarrow{x}_{n+1} = \overrightarrow{x}_n + \Delta \overrightarrow{x} = \overrightarrow{x}_n + \Delta t \overrightarrow{u}
//}
Can be expressed as

This method is called the forward Euler method.
By repeating this for each frame, the movement of particles at each time can be expressed.


== Fluid simulation by SPH method
In the previous section, we explained how to derive the NS equation in the particle method.
Of course, these differential equations cannot be solved directly by computer, so some kind of approximation is needed.
As a method, I will explain @<b>{SPH method} which is often used in the CG field.

The SPH method was originally used for the simulation of collisions between celestial bodies in astrophysics, but was also applied to fluid simulation in CG by Desbrun et al. @<fn>{desbrun} in 1996.
In addition, parallelization is easy, and with the current GPU, it is possible to calculate large numbers of particles in real time.
In computer simulation, it is necessary to discretize continuous physical quantities for calculation,
This discretization is@<b>{Weight function}The method that uses a function called is called the SPH method.

//footnote[desbrun][Desbrun and Cani, Smoothed Particles: A new paradigm for animating highly deformable bodies, Eurographics Workshop on Computer Animation and Simulation (EGCAS), 1996.]

=== Discretization of physical quantities

In the SPH method, each particle has a range of influence, and the closer the distance to other particles, the greater the influence of that particle.
When this influence range is illustrated@<img>{2dkernel}It looks like.
//image[2dkernel][2-D weight function][scale=0.5]{
//}
This function@<b>{Weight function}@<fn>{weight_fn}I call it.

//footnote[weight_fn][Usually, this function is also called a kernel function, but this name is used to distinguish it from the kernel function in ComputeShader.]

Physical quantity in SPH method@<m>{\phi}Then, it is discretized using the weight function as follows.
//texequation{
  \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j}W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
@<m>{N, m, \rho, h}Are the collection of neighboring particles, particle mass, particle density, and radius of influence of the weighting function, respectively.
Also, the function @<m>{W} is the weighting function mentioned earlier.

Furthermore, partial differential operations such as gradient and Laplacian can be applied to this physical quantity,
The gradient is
//texequation{
  \nabla \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j} \nabla W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
Laplacian is
//texequation{
  \nabla^2 \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j} \nabla^2 W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
Can be expressed as
As you can see from the formula, the gradient of the physical quantity and the Laplacian are images that are applied only to the weighting function.
Weight function@<m>{W}Uses different values ​​depending on the physical quantity you want to find, but I will omit the explanation for this reason.@<fn>{fujisawa}To do.
#@#Graph image of weight function
//footnote[fujisawa]["Basics of physical simulation for CG: Makoto Fujisawa".]

=== Discretization of density
The density of fluid particles can be calculated using the equation of physical quantity discretized by the weighting function.
//texequation{
  \rho(\overrightarrow{x}) = \sum_{j \in N}m_jW_{poly6}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
Is given.
Where the weighting function to use@<m>{W}Is given by
//image[poly6][Poly6 weight function][scale=0.7]{
//}

=== Discretization of viscosity term
The viscous term is discretized using the weighting function as in the case of density,
//texequation{
  f_{i}^{visc} = \mu\nabla^2\overrightarrow{u}_i = \mu \sum_{j \in N}m_j\frac{\overrightarrow{u}_j - \overrightarrow{u}_i}{\rho_j} \nabla^2 W_{visc}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
It is expressed as.
Where the Laplacian of the weighting function@<m>{\nabla^2 W_{visc\}}Is given by
//image[visc][Viscosity Laplacian of weighting function][scale=0.7]{
//}

=== Discretization of pressure term
Similarly, discretize the pressure term.
//texequation{
  f_{i}^{press} = - \frac{1}{\rho_i} \nabla p_i = - \frac{1}{\rho_i} \sum_{j \in N}m_j\frac{p_j - p_i}{2\rho_j} \nabla W_{spiky}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
Where the gradient of the weighting function@<m>{W_{spiky\}}Is given by
//image[spiky][Spiky weight function gradient][scale=0.7]{
//}

At this time, the particle pressure is called Tait equation in advance,
//texequation{
    p = B\left\{\left(\frac{\rho}{\rho_0}\right)^\gamma - 1\right\}
//}
It is calculated by. Where @<m>{B} is the gas constant.
In order to guarantee incompressibility, it is necessary to solve the Poisson equation, but it is not suitable for real-time calculation.
Instead, SPH method@<fn>{wcsph}Then, it is said that the calculation of the pressure term is weaker than the lattice method in terms of securing incompressibility approximately.

//footnote[wcsph][The SPH method that calculates pressure using the Tait equation is called the WCSPH method.]

== Implementation of SPH method
Sample here(@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming})Assets/SPH Fluid is listed below.
Please note that this implementation does not consider speedup or numerical stability in order to explain the SPH method as simply as possible.

=== The parameter
The comments in the code describe the various parameters used for the simulation.
//listnum[parameters][Parameters used for simulation(FluidBase.cs)][csharp]{
NumParticleEnum particleNum = NumParticleEnum.NUM_8K;    // Number of particles
float smoothlen = 0.012f;               // Particle radius
float pressureStiffness = 200.0f;       // Pressure term coefficient
float restDensity = 1000.0f;            // Resting density
float particleMass = 0.0002f;           // Particle mass
float viscosity = 0.1f;                 // Viscosity coefficient
float maxAllowableTimestep = 0.005f;    // Step size
float wallStiffness = 3000.0f;          // Power of Penalty Law Wall
int iterations = 4;                     // Number of iterations
Vector2 gravity = new Vector2(0.0f, -0.5f);     // gravity
Vector2 range = new Vector2(1, 1);              // Simulation space
bool simulate = true;                           // Execute or pause

int numParticles;              // Number of particles
float timeStep;                // Step size
float densityCoef;             // Density coefficient of Poly6 kernel
float gradPressureCoef;        // Pressure coefficient of Spiky kernel
float lapViscosityCoef;        // Laplacian kernel viscosity coefficient
//}

Please note that in this demo scene, the inspector is set to a value different from the initialization value of the parameter described in the code.

=== Compute SPH weight function coefficients
The coefficient of the weight function does not change during the simulation, so it should be calculated on the CPU side at initialization.
(However, it is updated in the Update function considering the possibility of editing the parameter during execution)

This time, the mass of each particle is kept constant, so the mass @<m>{m} in the physical quantity formula goes out of the sigma and becomes the following.
//texequation{
  \phi(\overrightarrow{x}) = m \sum_{j \in N}\frac{\phi_j}{\rho_j}W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
Therefore, the mass can be included in the coefficient calculation.

Since the coefficient changes depending on the type of weighting function, calculate the coefficient for each.

//listnum[coefs][Precompute coefficients for weighting function(FluidBase.cs)][csharp]{
densityCoef = particleMass * 4f / (Mathf.PI * Mathf.Pow(smoothlen, 8));
gradPressureCoef
    = particleMass * -30.0f / (Mathf.PI * Mathf.Pow(smoothlen, 5));
lapViscosityCoef
    = particleMass * 20f / (3 * Mathf.PI * Mathf.Pow(smoothlen, 5));
//}

Finally, the coefficients (and various parameters) calculated by these CPUs To GPU Store in the constant buffer on the side.
//listnum[setconst][ComputeShader Transfer the value to the constant buffer of(FluidBase.cs)][csharp]{
fluidCS.SetInt("_NumParticles", numParticles);
fluidCS.SetFloat("_TimeStep", timeStep);
fluidCS.SetFloat("_Smoothlen", smoothlen);
fluidCS.SetFloat("_PressureStiffness", pressureStiffness);
fluidCS.SetFloat("_RestDensity", restDensity);
fluidCS.SetFloat("_Viscosity", viscosity);
fluidCS.SetFloat("_DensityCoef", densityCoef);
fluidCS.SetFloat("_GradPressureCoef", gradPressureCoef);
fluidCS.SetFloat("_LapViscosityCoef", lapViscosityCoef);
fluidCS.SetFloat("_WallStiffness", wallStiffness);
fluidCS.SetVector("_Range", range);
fluidCS.SetVector("_Gravity", gravity);
//}

//listnum[const][ComputeShader Constant buffer(SPH2D.compute)][csharp]{
int   _NumParticles;      // Number of particles
float _TimeStep;          // Step size(dt)
float _Smoothlen;         // Particle radius
float _PressureStiffness; // Becker Coefficient of
float _RestDensity;       // Resting density
float _DensityCoef;       // Coefficient for calculating density
float _GradPressureCoef;  // Coefficient when calculating pressure
float _LapViscosityCoef;  // Coefficient when calculating viscosity
float _WallStiffness;     // Force of pushing back the penalty method
float _Viscosity;         // Viscosity coefficient
float2 _Gravity;          // gravity
float2 _Range;            // Simulation space

float3 _MousePos;         // Mouse position
float _MouseRadius;       // Radius of mouse interaction
bool _MouseDown;          // Is the mouse pressed?
//}


=== Density calculation
//listnum[density_kernel][Kernel function for calculating density(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void DensityCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x;	// Particle ID currently being processed

	float h_sq = _Smoothlen * _Smoothlen;
	float2 P_position = _ParticlesBufferRead[P_ID].position;

	// Neighborhood search(O(n^2))
	float density = 0;
	for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
		if (N_ID == P_ID) continue;	// Avoid referencing yourself

		float2 N_position = _ParticlesBufferRead[N_ID].position;

		float2 diff = N_position - P_position;    // 粒子距離
		float r_sq = dot(diff, diff);             // 粒子距離の2乗

		// Exclude particles that are not within the radius
		if (r_sq < h_sq) {
            // No need to take a route as the calculation only includes squares
			density += CalculateDensity(r_sq);
		}
	}

	// Update density buffer
	_ParticlesDensityBufferWrite[P_ID].density = density;
}
//}

Originally, it is necessary to search for neighboring particles using an appropriate neighborhood search algorithm without exhaustive examination of all particles.
For the sake of simplicity, this implementation implements a 100% survey (for loop on line 10).
Also, since the distance between yourself and the other particle is calculated, you avoid doing calculations between your own particles on line 11.

Effective radius of weight function@<m>{h}It is realized by the if statement on the 19th line.
Addition of densities (calculation of sigma) is realized by adding the calculation result inside sigma to the variable initialized with 0 in the 9th line.
Here is the density formula again.
//texequation{
  \rho(\overrightarrow{x}) = \sum_{j \in N}m_jW_{poly6}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
The density calculation uses the Poly6 weighting function as shown in the above formula. Poly6 weighting function is@<list>{density_weight}Calculate with.
//listnum[density_weight][密度の計算(SPH2D.compute)][c]{
inline float CalculateDensity(float r_sq) {
	const float h_sq = _Smoothlen * _Smoothlen;
	return _DensityCoef * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
}
//}

Finally@<list>{density_kernel}In the 25th line of, write to the write buffer.

=== Calculation of pressure in particles
//listnum[press_kernel][Weight function to calculate the pressure for each particle(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void PressureCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x;	// Particle ID currently being processed

	float  P_density = _ParticlesDensityBufferRead[P_ID].density;
	float  P_pressure = CalculatePressure(P_density);

	// Update pressure buffer
	_ParticlesPressureBufferWrite[P_ID].pressure = P_pressure;
}
//}

Before solving the pressure term, calculate the pressure in particle units, and reduce the calculation cost of the pressure term after that.
As I mentioned earlier, it is necessary to solve the equation called Poisson's equation like the following in the calculation of pressure.
//texequation{
    \nabla^2 p = \rho \frac{\nabla \overrightarrow{u}}{\Delta t}
//}
However, the operation to solve the Poisson equation accurately with a computer is very expensive, so it is approximately calculated using the following Tait equation.
//texequation{
    p = B\left\{\left(\frac{\rho}{\rho_0}\right)^\gamma - 1\right\}
//}
//listnum[tait][Implementation of Tait equation(SPH2D.compute)][c]{
inline float CalculatePressure(float density) {
	return _PressureStiffness * max(pow(density / _RestDensity, 7) - 1, 0);
}
//}


=== Calculation of pressure and viscosity terms
//listnum[force_kernel][Kernel function that calculates pressure and viscosity terms(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void ForceCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x; // Particle ID currently being processed

	float2 P_position = _ParticlesBufferRead[P_ID].position;
	float2 P_velocity = _ParticlesBufferRead[P_ID].velocity;
	float  P_density = _ParticlesDensityBufferRead[P_ID].density;
	float  P_pressure = _ParticlesPressureBufferRead[P_ID].pressure;

	const float h_sq = _Smoothlen * _Smoothlen;

	// Neighborhood search(O(n^2))
	float2 press = float2(0, 0);
	float2 visco = float2(0, 0);
	for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
		if (N_ID == P_ID) continue;	// Skip if you target yourself

		float2 N_position = _ParticlesBufferRead[N_ID].position;

		float2 diff = N_position - P_position;
		float r_sq = dot(diff, diff);

		// Exclude particles that are not within the radius
		if (r_sq < h_sq) {
			float  N_density
                    = _ParticlesDensityBufferRead[N_ID].density;
			float  N_pressure
                    = _ParticlesPressureBufferRead[N_ID].pressure;
			float2 N_velocity
                    = _ParticlesBufferRead[N_ID].velocity;
			float  r = sqrt(r_sq);

			// Pressure item
			press += CalculateGradPressure(...);

			// Sticky item
			visco += CalculateLapVelocity(...);
		}
	}

	// Integration
	float2 force = press + _Viscosity * visco;

	// Acceleration buffer update
	_ParticlesForceBufferWrite[P_ID].acceleration = force / P_density;
}
//}

The pressure and viscosity terms are calculated in the same way as the density calculation method.

First, we calculate the force by the following pressure term in line 31.
//texequation{
  f_{i}^{press} = - \frac{1}{\rho_i} \nabla p_i = - \frac{1}{\rho_i} \sum_{j \in N}m_j\frac{p_j - p_i}{2\rho_j} \nabla W_{press}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
The following function calculates the contents of Sigma.
//listnum[press_weight][圧力項の要素の計算(SPH2D.compute)][c]{
inline float2 CalculateGradPressure(...) {
	const float h = _Smoothlen;
	float avg_pressure = 0.5f * (N_pressure + P_pressure);
	return _GradPressureCoef * avg_pressure / N_density
            * pow(h - r, 2) / r * (diff);
}
//}

Next, the calculation of the force by the following viscous term is performed on the 34th line.
//texequation{
  f_{i}^{visc} = \mu\nabla^2\overrightarrow{u}_i = \mu \sum_{j \in N}m_j\frac{\overrightarrow{u}_j - \overrightarrow{u}_i}{\rho_j} \nabla^2 W_{visc}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
The following function calculates the contents of Sigma.
//listnum[visc_weight][Calculate elements of viscosity term(SPH2D.compute)][c]{
inline float2 CalculateLapVelocity(...) {
	const float h = _Smoothlen;
	float2 vel_diff = (N_velocity - P_velocity);
	return _LapViscosityCoef / N_density * (h - r) * vel_diff;
}
//}

Finally,@<list>{force_kernel}At line 39, the forces calculated by the pressure term and the viscosity term are added together and written in the buffer as the final output.

=== Collision judgment and position update
//listnum[integrate_kernel][Kernel function for collision detection and position update(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void IntegrateCS(uint3 DTid : SV_DispatchThreadID) {
	const unsigned int P_ID = DTid.x; // Particle ID currently being processed

	// Position and speed before update
	float2 position = _ParticlesBufferRead[P_ID].position;
	float2 velocity = _ParticlesBufferRead[P_ID].velocity;
	float2 acceleration = _ParticlesForceBufferRead[P_ID].acceleration;

	// Mouse interaction
	if (distance(position, _MousePos.xy) < _MouseRadius && _MouseDown) {
		float2 dir = position - _MousePos.xy;
		float pushBack = _MouseRadius-length(dir);
		acceleration += 100 * pushBack * normalize(dir);
	}

	// If you want to write collision judgment, here -----

	// Wall boundary (penalty method)
	float dist = dot(float3(position, 1), float3(1, 0, 0));
	acceleration += min(dist, 0) * -_WallStiffness * float2(1, 0);

	dist = dot(float3(position, 1), float3(0, 1, 0));
	acceleration += min(dist, 0) * -_WallStiffness * float2(0, 1);

	dist = dot(float3(position, 1), float3(-1, 0, _Range.x));
	acceleration += min(dist, 0) * -_WallStiffness * float2(-1, 0);

	dist = dot(float3(position, 1), float3(0, -1, _Range.y));
	acceleration += min(dist, 0) * -_WallStiffness * float2(0, -1);

	// Addition of gravity
	acceleration += _Gravity;

	// Update the next particle position with the forward Euler method
	velocity += _TimeStep * acceleration;
	position += _TimeStep * velocity;

	// Particle buffer update
	_ParticlesBufferWrite[P_ID].position = position;
	_ParticlesBufferWrite[P_ID].velocity = velocity;
}
//}

Use the penalty method to judge collision with a wall(19-30 Line)。
The penalty method is a method of pushing back with a strong force as much as it sticks out from the boundary position.

Originally, the collision judgment with the obstacle is also performed before the collision judgment with the wall, but in this implementation, the interaction with the mouse is performed.(213-218 Line)。
If the mouse is pressed, the specified force is applied to move away from the mouse position.

In line 33, the external force of gravity is added.
Setting the gravity value to zero creates weightlessness, which is an interesting visual effect.
In addition, the position is updated by the forward Euler method described above.(36-37 Line)、Write the final result to the buffer.


=== Simulation main routine
//listnum[routine][Simulation main functions(FluidBase.cs)][csharp]{
private void RunFluidSolver() {

  int kernelID = -1;
  int threadGroupsX = numParticles / THREAD_SIZE_X;

  // Density
  kernelID = fluidCS.FindKernel("DensityCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Pressure
  kernelID = fluidCS.FindKernel("PressureCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesPressureBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Force
  kernelID = fluidCS.FindKernel("ForceCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesPressureBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesForceBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Integrate
  kernelID = fluidCS.FindKernel("IntegrateCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesForceBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  SwapComputeBuffer(ref particlesBufferRead, ref particlesBufferWrite);
}
//}
This is the part that calls the kernel function of ComputeShader described so far every frame.
Give an appropriate ComputeBuffer for each kernel function.

Where time step width@<m>{\Delta t}Recall that the smaller the, the less error in the simulation.
When running at 60FPS,@<m>{\Delta t = 1 / 60}However, this will cause a large error and the particles will explode.
further,@<m>{\Delta t = 1 / 60}If the time step width is smaller, the progress of time per frame will be slower than the actual time, resulting in slow motion.
To avoid this,@<m>{\Delta t = 1 / (60 \times {iterarion\})}As for, iterarion turns the main routine once per frame.

//listnum[iteration][Iteration of major functions(FluidBase.cs)][csharp]{
// Iterate multiple times with smaller time step to improve calculation accuracy
for (int i = 0; i<iterations; i++) {
    RunFluidSolver();
}
//}
By doing this, you can perform real-time simulation with a small time step width.

=== How to use the buffer
Unlike the normal single access particle system,
Since particles interact with each other, it would be a problem if other data were rewritten during the calculation.
In order to avoid this, prepare two buffers, a read buffer and a write buffer, that do not rewrite the values ​​when performing calculations on the GPU.
By exchanging these buffers every frame, data can be updated without conflict.
//listnum[swap][Buffer swapping function(FluidBase.cs)][csharp]{
void SwapComputeBuffer(ref ComputeBuffer ping, ref ComputeBuffer pong) {
    ComputeBuffer temp = ping;
    ping = pong;
    pong = temp;
}
//}

=== Particle rendering
//listnum[rendercs][Particle rendering(FluidRenderer.cs)][csharp]{
void DrawParticle() {

  Material m = RenderParticleMat;

  var inverseViewMatrix = Camera.main.worldToCameraMatrix.inverse;

  m.SetPass(0);
  m.SetMatrix("_InverseMatrix", inverseViewMatrix);
  m.SetColor("_WaterColor", WaterColor);
  m.SetBuffer("_ParticlesBuffer", solver.ParticlesBufferRead);
  Graphics.DrawProcedural(MeshTopology.Points, solver.NumParticles);
}
//}
On the 10th line, set the buffer that stores the position calculation results of the fluid particles in the material and transfer it to the shader.
In the 11th line, we are instructing to draw instances for the number of particles.

//listnum[render][Particle rendering(Particle.shader)][c]{
struct FluidParticle {
	float2 position;
	float2 velocity;
};

StructuredBuffer<FluidParticle> _ParticlesBuffer;

// --------------------------------------------------------------------
// Vertex Shader
// --------------------------------------------------------------------
v2g vert(uint id : SV_VertexID) {

	v2g o = (v2g)0;
	o.pos = float3(_ParticlesBuffer[id].position.xy, 0);
	o.color = float4(0, 0.1, 0.1, 1);
	return o;
}
//}
1-6 In the line, define the information to receive the fluid particle information.
At this time, it is necessary to match the definition with the structure of the buffer transferred from the script to the material.
The position data is received as in line 14. id : SV_VertexID This is done by referring to the buffer element with.

After that, just like a normal particle system@<img>{bill}like
Billboard centered on the position data of the calculation result with the geometry shader@<fn>{billboard}Create
Attach the particle image and render.
//image[bill][Billboard Creation][scale=1]{
//}

//footnote[billboard][Plane where the table always faces the viewpoint.]

== result
//image[result][Rendering result]{
//}

Click here for video(@<href>{https://youtu.be/KJVu26zeK2w})It is posted in.

== Summary
In this chapter, we showed the method of fluid simulation using SPH method.
By using the SPH method, it has become possible to handle fluid movements in a general-purpose manner like a particle system.

As mentioned above, there are many types of fluid simulation methods other than the SPH method.
Throughout this chapter, you will be interested in other physical simulations in addition to other fluid simulation methods.
We would appreciate if you could expand the range of expressions.