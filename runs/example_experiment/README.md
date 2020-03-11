# Example pipeline

This run folder is an example of how one could setup an experiment.

## Experiment details

This experiment details are as follows:

* Topology: a ToR with 4 servers
* Each link has a rate of 100 Mbit/s. If you run a single flow from A to B over a 100 Mbit/s link, it gets about 86.8 Mbit/s as goodput rate. (due to transport protocol / header overhead)
* Bi-section bandwidth: (86.8 * 4) Mbit/s
* Flows: source and destination are random uniformly picked ("all-to-all"), each is of size 100KB
* Maximum flow arrival rate supported under ideal conditions: (86.8 * 4) Mbit/s / 100KB = 434 flows/s 
* Flow inter-arrival time: drawn from Poisson process. Poisson flow arrival rate is varied: [10, 50, 100, 150, 200, 250, 300, 350, 400]
* Each run is performed 5 times with different schedule seeds

## Getting started

1. Execute:
   ```
   bash generate_runs.sh
   ```
   This will generate all the run folders in `runs/`
   
2. Execute:
   ```
   bash perform_runs.sh
   ```
   This will run the simulator one-by-one over the run folders in `runs/`
   
3. Execute:
   ```
   python analyze.py
   ```
   This will generate FCT statistics for each run folder (in `analysis/flows.statistics` of each run folder) and general statistics among them in `data/`
   
3. Execute:
   ```
   bash plot.sh
   ```
   This will use the files in `data/` to create a few plots in `pdf/`
