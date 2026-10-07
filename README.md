# K-Means Clustering Visualizer

An interactive **K-Means clustering visualizer built from scratch in C++17**.

## Goal

The goal of this project is to understand and implement the K-Means clustering algorithm without relying on machine-learning libraries.

The project will visualize how K-Means:

1. Initializes \(K\) centroids.
2. Assigns each point to its nearest centroid.
3. Recalculates each centroid using the mean of its assigned points.
4. Repeats these steps until the algorithm converges.

The visualizer will make the optimization process visible by showing cluster assignments, centroid movement, iterations, and the clustering objective.

## Learning Objectives

Through this project I aim to understand:

* Unsupervised learning and clustering
* Euclidean distance and squared distance
* Centroids and cluster assignments
* The K-Means objective function
* Why the mean minimizes squared distance
* Iterative optimization and convergence
* Local minima and initialization
* The effect of choosing different values of \(K\)
* Limitations of K-Means on non-spherical or irregularly shaped data

## Planned Features

* Generate 2D datasets
* Place points manually
* Choose \(K\)
* Randomly initialize centroids
* Visualize cluster assignments using colors
* Visualize centroid movement
* Step through individual iterations
* Run the algorithm continuously
* Display the current iteration and objective/loss
* Detect convergence
* Experiment with different datasets and initializations

## Implementation

* **Language:** C++17
* **Graphics/UI:** Raylib
* **Machine-learning implementation:** From scratch
* **External ML libraries:** None

The primary purpose of this project is **understanding the mathematics and mechanics of K-Means**, rather than building the most optimized clustering implementation possible.
