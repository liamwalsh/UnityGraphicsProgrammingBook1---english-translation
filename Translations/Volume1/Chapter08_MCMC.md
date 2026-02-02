# Chapter 8: MCMC 3D Space Sampling

*Original author: @komietty*
*Translation and annotations by Claude*

---

## Introduction

This chapter explains **MCMC (Markov Chain Monte Carlo)**, a sampling method that draws multiple values from a given probability distribution.

The simplest method for sampling from a probability distribution is **rejection sampling**, but in 3D space, the rejection region becomes very large, making it impractical. MCMC enables efficient sampling even in high-dimensional spaces.

> **The Information Gap**
>
> MCMC information tends to fall into two camps: academic books with rigorous theory but no implementation guidance, or online code snippets with no theoretical background. This chapter aims to bridge that gap—providing enough theory to understand what you're implementing, without getting lost in mathematical formalism.

The probability concepts needed here are kept minimal. We'll aim for intuitive understanding rather than mathematical rigor.

---

## Sample Repository

Sample code is in **Assets/MCMC3D** at:
https://github.com/IndieVisualLab/UnityGraphicsProgramming

---

## Probability Fundamentals

To understand MCMC, you only need four concepts:

1. Random Variable
2. Probability Distribution
3. Stochastic Process
4. Stationary Distribution

### Random Variable

When an event occurs with probability P(X), the value X is called a **random variable**. For example: "The probability of rolling a 5 on a die is 1/6"—here "5" is the random variable and "1/6" is the probability.

More formally: a random variable X is a mapping X = X(ω) from the sample space Ω (all possible outcomes) to real numbers.

### Stochastic Process

A **stochastic process** adds a time dimension to random variables: X = X(ω, t). It's essentially a random variable that evolves over time.

### Probability Distribution

A **probability distribution** shows the relationship between random variable X and probability P(X). Often visualized as a graph with X on the horizontal axis and P(X) on the vertical axis.

### Stationary Distribution

A **stationary distribution** is one where individual points may transition, but the overall distribution shape remains unchanged. For distribution P and transition matrix π, if πP = P, then P is a stationary distribution.

> **Visual Intuition**
>
> Imagine particles bouncing around according to some rules. Even though each particle moves, if the overall "cloud" of particles maintains the same shape, that shape is the stationary distribution.

---

## MCMC Concepts

MCMC samples from a given distribution that is assumed to be stationary, using **Monte Carlo** methods combined with **Markov chains**.

### Monte Carlo Methods

Monte Carlo methods use pseudo-random numbers for numerical computation or simulation.

A classic example—estimating π:

```csharp
float pi;
float trial = 10000;
float count = 0;

for(int i = 0; i < trial; i++){
    float x = Random.value;
    float y = Random.value;
    if(x*x + y*y <= 1) count++;
}

pi = 4 * count / trial;
```

This randomly samples points in a unit square and counts how many fall inside a quarter circle. The ratio approximates π/4.

### Markov Chains

A **Markov chain** is a stochastic process where **future states depend only on the current state, not on past history**. This is called the **Markov property**.

> **"Memoryless"**
>
> The Markov property means the system has no memory. Where you came from doesn't matter—only where you are now determines where you can go next.

### Convergence to Stationary Distribution

For MCMC to work, the sampling process must converge to the target distribution. This requires two conditions:

**1. Irreducibility**: The distribution cannot be split into disconnected parts. From any point, you must be able to reach any other point through transitions.

**2. Aperiodicity**: You can return to any state in any number of steps. No periodic patterns like "can only return in even numbers of steps."

When both conditions are met, any initial distribution converges to the target stationary distribution. This is called **ergodicity**.

### The Metropolis Algorithm

Checking ergodicity directly is tedious, so we use a stronger condition called **detailed balance**. The Metropolis algorithm is one method that satisfies detailed balance.

**Metropolis algorithm steps:**

1. **Propose**: Generate a candidate next state x' using a proposal distribution Q where Q(x|x') = Q(x'|x) (symmetric). Gaussian distributions are commonly used.

2. **Accept/Reject**: Generate a uniform random number r ∈ [0,1). If P(x')/P(x) > r, accept the transition to x'. Otherwise, stay at x.

> **Why This Works**
>
> The acceptance criterion means:
> - Higher probability regions are always accepted
> - Lower probability regions are accepted with probability proportional to the ratio
>
> This lets the sampler explore the distribution while spending more time in high-probability regions—exactly what we want for sampling.

The Metropolis algorithm is a special case of **Metropolis-Hastings** that uses symmetric proposal distributions.

---

## 3D Sampling Implementation

Let's implement MCMC for 3D space sampling.

### Creating the Target Distribution

First, create a 3D probability distribution to sample from:

```csharp
void Prepare()
{
    var sn = new SimplexNoiseGenerator();
    for (int x = 0; x < lEdge; x++)
        for (int y = 0; y < lEdge; y++)
            for (int z = 0; z < lEdge; z++)
            {
                var i = x + lEdge * y + lEdge * lEdge * z;
                var val = sn.noise(x, y, z);
                data[i] = new Vector4(x, y, z, val);
            }
}
```

This example uses **Simplex noise** as the target distribution—a continuous 3D scalar field with varying density.

### Running MCMC

```csharp
public IEnumerable<Vector3> Sequence(int nInit, int limit, float th)
{
    Reset();

    // Burn-in: discard initial samples (not yet converged)
    for (var i = 0; i < nInit; i++)
        Next(th);

    // Main sampling loop
    for (var i = 0; i < limit; i++)
    {
        yield return _curr;
        Next(th);
    }
}

public void Reset()
{
    // Find a valid starting point
    for (var i = 0; _currDensity <= 0f && i < limitResetLoopCount; i++)
    {
        _curr = new Vector3(
            Scale.x * Random.value,
            Scale.y * Random.value,
            Scale.z * Random.value
        );
        _currDensity = Density(_curr);
    }
}
```

Using coroutines allows the process to run across frames. MCMC can be thought of as conceptually parallel—each chain explores independently.

> **Burn-in Period**
>
> Early samples may be far from the target distribution. We discard the first `nInit` samples (burn-in) to let the chain converge before collecting actual samples.

### Proposal Distribution

For 3D sampling, use a trivariate standard normal distribution:

```csharp
public static Vector3 GenerateRandomPointStandard()
{
    var x = RandomGenerator.rand_gaussian(0f, 1f);
    var y = RandomGenerator.rand_gaussian(0f, 1f);
    var z = RandomGenerator.rand_gaussian(0f, 1f);
    return new Vector3(x, y, z);
}

public static float rand_gaussian(float mu, float sigma)
{
    // Box-Muller transform for Gaussian random numbers
    float z = Mathf.Sqrt(-2.0f * Mathf.Log(Random.value))
              * Mathf.Sin(2.0f * Mathf.PI * Random.value);
    return mu + sigma * z;
}
```

For non-standard covariance, use Cholesky decomposition:

```csharp
public static Vector3 GenerateRandomPoint(Matrix4x4 sigma)
{
    // Cholesky decomposition for correlated Gaussian
    var c00 = sigma.m00 / Mathf.Sqrt(sigma.m00);
    var c10 = sigma.m10 / Mathf.Sqrt(sigma.m00);
    // ... (decomposition continues)

    var r1 = RandomGenerator.rand_gaussian(0f, 1f);
    var r2 = RandomGenerator.rand_gaussian(0f, 1f);
    var r3 = RandomGenerator.rand_gaussian(0f, 1f);

    var x = c00 * r1;
    var y = c10 * r1 + c11 * r2;
    var z = c20 * r1 + c21 * r2 + c22 * r3;
    return new Vector3(x, y, z);
}
```

### Transition Decision

The core Metropolis step:

```csharp
void Next(float threshold)
{
    // Propose: current position + Gaussian offset
    Vector3 next = GaussianDistributionCubic.GenerateRandomPointStandard()
                   + _curr;

    var densityNext = Density(next);

    // Accept if: (1) current density is zero, OR
    //           (2) ratio exceeds random threshold
    bool flag1 = _currDensity <= 0f ||
                 Mathf.Min(1f, densityNext / _currDensity) >= Random.value;

    // Also require minimum density threshold
    bool flag2 = densityNext > threshold;

    if (flag1 && flag2)
    {
        _curr = next;
        _currDensity = densityNext;
    }
}
```

### Density Estimation

Looking up exact density at arbitrary coordinates in a discrete grid is expensive (O(n³)). Instead, approximate using weighted average of nearby samples:

```csharp
float Density(Vector3 pos)
{
    float weight = 0f;
    for (int i = 0; i < weightReferenceloopCount; i++)
    {
        // Random sampling from data array
        int id = (int)Mathf.Floor(Random.value * (Data.Length - 1));
        Vector3 posi = Data[id];
        float mag = Vector3.SqrMagnitude(pos - posi);

        // Inverse distance weighting (exponential falloff)
        weight += Mathf.Exp(-mag) * Data[id].w;
    }
    return weight;
}
```

This uses stochastic sampling with exponential distance weighting for fast approximation.

---

## Additional Notes

The repository includes a **rejection sampling** implementation for comparison. With rejection sampling, strong threshold values cause most samples to be rejected. MCMC produces similar results much more smoothly.

**Step size control**: By reducing the random walk step size, consecutive samples stay close together. This can be used to simulate clustered phenomena like plant or flower colonies.

---

## Summary

MCMC enables efficient sampling from complex probability distributions in high-dimensional spaces. The key insights are:

1. Markov chains explore the space with "no memory"
2. Detailed balance ensures convergence to target distribution
3. Metropolis acceptance criterion balances exploration and exploitation
4. Burn-in discards non-converged initial samples

---

## Key Takeaways

> **What You Should Remember**
>
> 1. **MCMC = Monte Carlo + Markov Chain**: Random sampling with memoryless transitions
>
> 2. **Markov property**: Future depends only on present, not past
>
> 3. **Stationary distribution**: Overall shape stays constant despite individual transitions
>
> 4. **Ergodicity**: Irreducibility + Aperiodicity → convergence guaranteed
>
> 5. **Metropolis algorithm**: Propose → Accept/Reject based on probability ratio
>
> 6. **Burn-in**: Discard early samples before convergence
>
> 7. **Step size**: Smaller steps → clustered samples; larger steps → more exploration

---

## MCMC vs Rejection Sampling

| Aspect | Rejection Sampling | MCMC |
|--------|-------------------|------|
| **Efficiency** | Low in high dimensions | Scales well |
| **Correlation** | Independent samples | Correlated (chain) |
| **Convergence** | N/A | Needs burn-in |
| **Implementation** | Simple | More complex |
| **Use case** | Low dimensions | High dimensions |

---

## Applications

MCMC is used in:
- Bayesian inference
- Physics simulations
- Procedural content generation
- Machine learning
- Financial modeling
- Molecular dynamics

---

## References

- Kubo Takuya (2012) "Introduction to Statistical Modeling for Data Analysis"
- Olle Häggström (2017) "Gentle Introduction to MCMC"

---

*Next chapter: Procedural Modeling!*
