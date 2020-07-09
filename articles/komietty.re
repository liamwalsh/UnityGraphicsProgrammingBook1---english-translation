
= 3D spatial sampling performed by MCMC

== Introduction


In this chapter, we will explain the sampling method. This time, we will focus on a sampling method called MCMC (Markov chain Monte Carlo method) that samples appropriate values ​​from a certain probability distribution.



The simplest method for sampling from a certain probability distribution is the rejection method. However, sampling in a three-dimensional space causes a large rejected area and cannot be used in actual operation. Therefore, it is the content of this chapter that MCMC can be used for efficient sampling even in high dimensions.



On the one hand, the information about MCMC is for books such as books, which is for statisticians, but it is redundant for programmers, but there is no guide to implementation. The fact is that there is no content to understand the theory and implementation quickly and in a comprehensive manner, as it is only described and there is no care for the theoretical background. I have tried to make the concrete explanations in the following sections as much as possible.



The explanation of the probability that is the background of MCMC is such that it is possible to write one book if strict. This time, with the motto of explaining the minimum theoretical background that can be implemented with peace of mind, the rigor of the definition was moderate, and the aim was to be as intuitive as possible. Mathematics is about the first year of university, and I think that the program can be read without difficulty by those who have used it a little for work.



== Sample repository


In this chapter, Unity Project of Unity Graphics Programming https://github.com/IndieVisualLab/UnityGraphicsProgramming内にあるAssets/ProceduralModeling以下をサンプルプログラムとして用意しています。
The translated code (comments in English can be found here https://github.com/LIAMPUTCODEHERE) TODO:


== Basic knowledge about probability


To understand the theory of MCMC, it is first necessary to suppress the basic contents of probability.
However, there are few concepts that should be held in order to understand MCMC, and there are only the following four. No likelihood or probability density function is needed!

 * Random variable
 * Probability distribution
 * Stochastic process
 * Stationary distribution



Let's look at them in order.


=== Random variable


This real number X when an event occurs at the probability P(X) is called a random variable. For example, when saying "the probability of getting a 5 on the die is 1/6", "5" is the random variable and "1/6" is the probability. In other words, the above sentence can be rephrased as follows: "The probability that an X on the die rolls is P(X)".



By the way, if we write it a bit like a definition, the random variable X is a map X that returns a real number X for the element ω (=one event that occurred) selected from the sample space Ω (= all the events that may occur). = X(ω) can be written.


=== Stochastic process


I added a slightly confusing definition in the latter half of the random variable because the assumption that the random variable X is expressed as X = X(ω) simplifies the understanding of the stochastic process. The stochastic process is the one obtained by adding the time condition to the previous X and can be expressed as X = X(ω, t). In other words, the stochastic process can be considered as a kind of random variable with the condition of time.


=== Probability distribution


The probability distribution shows the correspondence between the random variable X and the probability P(X). It is often expressed as a graph with probability P(X) on the vertical axis and X on the horizontal axis.


=== Stationary distribution


Each point is a distribution in which the overall distribution remains unchanged even after a transition. For a distribution P and some transition matrix π, P that satisfies πP = P is called a stationary distribution. This definition is hard to understand, but it's clear from the figure below.



//image[komiettyfig04][stationaryDistribution]{
//}



== MCMC concept


In this section, I will touch on the concepts that make up MCMC.@<br>{}
As mentioned at the beginning, MCMC is a method of sampling an appropriate value from a certain probability distribution, but more concretely, the Monte Carlo method It refers to the method of sampling by (Monte Carlo) and Markov chain. In the following, we will explain in order of Monte Carlo method, Markov chain, and stationary distribution.


=== Monte Carlo method


The Monte Carlo method is a general term for numerical calculation and simulation using pseudo random numbers. @<br>{}
An example that is often used to introduce numerical calculations by the Monte Carlo method is the calculation of pi as shown below.

//emlist{
float pi;
float trial = 10000;
float count = 0;

for(int i=0; i<trial; i++){
    float x = Random.value;
    float y = Random.value;
    if(x*x+y*y <= 1) count++;
}

pi = 4 * count / trial;
//}


In short, the ratio of the number of trials in a fan-shaped circle in a 1 x 1 square to the total number of trials is the area ratio, so the pi can be calculated from that. As a simple example, this is also the Monte Carlo method.


=== Markov chain


A Markov chain is a stochastic process that satisfies Markovity and whose states can be described discretely. @<br>{}
Markov property is a property in which the probability distribution of future states of a stochastic process depends only on the current state and not on the past states.



//image[komiettyfig01][MarkovChain]{
//}




In the Markov chain as shown above, the future state depends only on the present state, and does not directly affect the past state.


=== Stationary distribution


In MCMC, it is necessary to converge from a given distribution using pseudo-random numbers to a given stationary distribution. Because, if it does not converge to the given distribution, it will sample from a different distribution every time, and unless it is a stationary distribution, it will not be able to sample successfully in a chain. In order for an arbitrary distribution to converge to a given distribution, the following two conditions must be met.

 * Irreducibility: the condition that the distribution must not be divided into multiple parts. When repeating the transition from a certain point on the probability distribution, there must be no unreachable points



//image[komiettyfig02][Irreducibility]{
//}


 * Non-periodicity: The condition of returning to the original place n times for any n. For example, there should be no condition that only one skip can be made in the distribution lined up on the circumference.



//image[komiettyfig03][Aperiodicity]{
//}




As long as these two conditions are met, any given distribution can converge to the given stationary distribution. This is called the ergodic property of the Markov process.


=== Metropolis method


Now, it is difficult to check whether or not the given distribution satisfies the ergot characteristics mentioned earlier, so in many cases, we will strengthen the condition and investigate within the range of "detailed balance". One of the Markov chain methods that achieves a detailed balance is called the metropolis method.



The Metropolis method performs sampling in the following two steps

 1. Select a transition destination candidate x with a pseudo-random number. x is generated according to a distribution Q that satisfies Q(x|x') = Q(x'|x), and this distribution Q is called the proposed distribution. The Gaussian distribution is often chosen as the proposed distribution.
 1. A random number independent of 1 is generated, and if a certain criterion is satisfied using the random number, the transition destination candidate is adopted. Specifically, for a uniform random number 0 <= r <1, ​​the ratio P(x')/P(x of the probability value P(x) on the target distribution and the probability value P(x') of the transition candidate ) Satisfies P(x')/P(x)> r, transitions to the transition candidate destination.



The merit of the Metropolis method is that even after the transition to the maximum value of the probability distribution, if the value of r is small, the transition is to the smaller probability value, so sampling can be performed in proportion to the maximum value around the maximum value.



By the way, the Metropolis method is a type of Metropolis-Hasting method (MH method). The Metropolis method uses a symmetrical distribution for the proposed distribution, but the MH method does not have this limitation.


== Three-dimensional sampling


Now let's see how to implement MCMC while actually looking at the code excerpt.

First, prepare a three-dimensional probability distribution. This is called the target distribution. This is the “target” distribution because it is the distribution that you want to actually sample.


//emlist{
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
//}


This time, we used simplex noise as the target distribution.



Next, actually run MCMC.


//emlist{
public IEnumerable<Vector3> Sequence(int nInit, int limit, float th)
{
    Reset();

    for (var i = 0; i < nInit; i++)
        Next(th);

    for (var i = 0; i < limit; i++)
    {
        yield return _curr;
        Next(th);
    }
}
//}

//emlist{
public void Reset()
{
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
//}


Run the process using a coroutine. MCMC can be conceptually thought of as parallel processing, since processing starts at a completely different place when one Markov chain ends. This time, I use the Reset function to run another process after the series of processes is completed. By doing this, you will be able to perform good sampling even when there are many maximum values ​​of the probability distribution.



Since the first point after the transition is likely to be a point away from the target distribution, this section is discarded without sampling (burn-in). When the target distribution is sufficiently approached, sampling and transition are set a certain number of times, and then another series of processing is started.



Finally, the process of determining the transition.@<br>{}
Since it is three-dimensional, the proposed distribution uses the trivariate standard normal distribution as follows.


//emlist{
public static Vector3 GenerateRandomPointStandard()
{
        var x = RandomGenerator.rand_gaussian(0f, 1f);
        var y = RandomGenerator.rand_gaussian(0f, 1f);
        var z = RandomGenerator.rand_gaussian(0f, 1f);
        return new Vector3(x, y, z);
}
//}

//emlist{
public static float rand_gaussian(float mu, float sigma)
{
     float z = Mathf.Sqrt(-2.0f * Mathf.Log(Random.value))
              * Mathf.Sin(2.0f * Mathf.PI * Random.value);
     return mu + sigma * z;
}
//}


The Metropolis method requires a symmetrical distribution, so there is no need to set the mean value to anything other than 0, but if the variance is to be other than 1, use Cholesky decomposition to derive it as follows. I will.


//emlist{
public static Vector3 GenerateRandomPoint(Matrix4x4 sigma)
{
    var c00 = sigma.m00 / Mathf.Sqrt(sigma.m00);
    var c10 = sigma.m10 / Mathf.Sqrt(sigma.m00);
    var c20 = sigma.m21 / Mathf.Sqrt(sigma.m00);
    var c11 = Mathf.Sqrt(sigma.m11 - c10 * c10);
    var c21 = (sigma.m21 - c20 * c10) / c11;
    var c22 = Mathf.Sqrt(sigma.m22 - (c20 * c20 + c21 * c21));
    var r1 = RandomGenerator.rand_gaussian(0f, 1f);
    var r2 = RandomGenerator.rand_gaussian(0f, 1f);
    var r3 = RandomGenerator.rand_gaussian(0f, 1f);
    var x = c00 * r1;
    var y = c10 * r1 + c11 * r2;
    var z = c20 * r1 + c21 * r2 + c22 * r3;
    return new Vector3(x, y, z);
}
//}


The transition destination is determined by taking the ratio of the probabilities of the proposed distribution (which is one point above) next and the immediately preceding point _curr on the target distribution, and transitioning if it is greater than a uniform random number, and not transitioning otherwise. I will. @<br>{}
The probability value is approximated because it takes a lot of processing to find the probability value corresponding to the transition destination coordinate (the processing amount of O(n^3)). Since the target distribution uses a distribution that changes continuously this time, the probability value is approximately derived by performing a weighted average that is inversely proportional to the distance.


//emlist{
void Next(float threshold)
{
        Vector3 next =
          GaussianDistributionCubic.GenerateRandomPointStandard()
          + _curr;

        var densityNext = Density(next);
        bool flag1 =
          _currDensity <= 0f ||
          Mathf.Min(1f, densityNext / _currDensity) >= Random.value;
        bool flag2 = densityNext > threshold;
        if (flag1 && flag2)
        {
                _curr = next;
                _currDensity = densityNext;
        }
}

float Density(Vector3 pos)
{
        float weight = 0f;
        for (int i = 0; i < weightReferenceloopCount; i++)
        {
                int id = (int)Mathf.Floor(Random.value * (Data.Length - 1));
                Vector3 posi = Data[id];
                float mag = Vector3.SqrMagnitude(pos - posi);
                weight += Mathf.Exp(-mag) * Data[id].w;
        }
        return weight;
}
//}

== Other


This time, the repository also contains a sample of the three-dimensional rejection method (a simple Monte Carlo method as shown in the circle example), so you can compare it. In the rejection method, if the reference value for rejection is set to be strong, the sampling cannot be done well, but MCMC can present similar sampling results more smoothly. In MCMC, if you narrow the width of the random walk for each step, you can easily reproduce plant and flower colonies because it samples from close spaces in a series of chains.


== references
 * Kubo Takuya (2012) Introduction to statistical modeling for data analysis: generalized linear model, hierarchical Bayesian model, MCMC (science of probability and information) Iwanami Shoten
 * Olle Haggstrom, Kentaro Nomaguchi (2017) Introduction to Easy MCMC: Finite Markov Chain and Algorithm Kyoritsu Publishing
