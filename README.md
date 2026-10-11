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

## Features

* Add points to the 2D map with the mouse or load point coordinates from a
  data file.
* Generate a fresh deterministic random dataset from the setup screen or the
  **NEW DATASET** button (`G`) without changing the selected K or
  initialization method.
* Choose random, blobs, elongated clusters, two moons, or ring datasets.
  Adjust the generated point count with Up/Down on the setup screen.
* Choose the number of clusters \(K\) and initialize centroids using:
  * **Randomized** positions.
  * **K-means++**, which selects seeds from the dataset using squared-distance
    weighting. If points are added after choosing this mode, seeding is
    deferred until the first iteration.
  * **Manual** placement on the map.
* View cluster assignments, centroid movement, iteration count, and clustering
  cost.
* Step through iterations or run automatically. Automatic run starts slowly
  (one iteration every 2.5 seconds); use `[` / `]` to change its speed.
* Run multiple independent initializations and compare their costs and best
  clustering.
* View the final clustering map from the convergence summary.
* **Soft reset** to rerun with the same points, \(K\), and initialization
  method.
* **Hard reset** to keep the points while changing \(K\) or the initialization
  method. The main menu starts over with an empty dataset.
* Pan and zoom the map.

## Controls

| Action | Control |
| --- | --- |
| Add a data point | Left-click the map |
| Step one iteration | `Space` or **STEP** |
| Start / stop automatic run | `R` or **RUN / STOP** |
| Adjust automatic run speed | `[` / `]` or **- [** / **+ ]** |
| Soft reset | `Esc` or **SOFT RESET** |
| Change \(K\) or initialization method, keeping points | **HARD RESET** |
| Open multi-run setup | `M` or **MULTI-RUN** |
| Open setup | `S` or **SETUP** |
| Return to the main menu | `M` / `Esc` in result screens or **MAIN MENU** |
| Reset map view | `F2` |
| Pan / zoom | Middle-drag / mouse wheel |
| Open help | `F1` or `H` |
| Load point coordinates | `L` or **LOAD (L)**, then enter a file path |
| Generate a fresh random dataset | `G` or **NEW DATA (G)** |
| Randomize centroid seed on setup | `P` or **RANDOMIZE (P)** |
| Choose dataset type | Setup cards or `4`-`8` |
| Change generated point count | `Up` / `Down` on setup |
| Change blob count | `,` / `.` or the blob +/- controls on setup |
| Change standard deviation | `J` / `K` or the standard-deviation +/- controls on setup |
| Scroll multi-run costs | Mouse wheel or `Up` / `Down` on the results screen |

When the run converges, a summary screen shows the final metrics and map.
Multi-run results also show the best final clustering map.
In manual initialization, click to place centroids; use `Backspace` to undo
the last placement and `R` to clear placements.

## Implementation

* **Language:** C++17
* **Graphics/UI:** Raylib
* **Machine-learning implementation:** From scratch
* **External ML libraries:** None

The primary purpose of this project is **understanding the mathematics and mechanics of K-Means**, rather than building the most optimized clustering implementation possible.
