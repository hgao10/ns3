# Example pipeline

This run folder is an example of how one could setup an experiment.


## Experiment details

This experiment details are as follows:

* Topology: a ToR with 4 servers
* Each link has a rate of 100 Mbit/s. If you run a single flow from A to B over a 100 Mbit/s link, it gets about 86.8 Mbit/s as goodput rate. (due to transport protocol / header overhead)
* Bi-section bandwidth: (86.8 * 4) Mbit/s
* Flows size: fixed to 100KB
* Maximum flow arrival rate supported under ideal conditions: (86.8 * 4) Mbit/s / 100KB = 434 flows/s (this is the **saturation point**, beyond which flows won't finish)
* Arrival rate is varied on the x-axis: [10, 50, 100, 150, 200, 250, 300, 350, 400]

There are two schedule generators:

**Poisson schedule**
* Source and destination are random uniformly picked ("all-to-all")
* Flow inter-arrival time: drawn from Poisson process
* Each run is performed 5 times with different schedule seeds
* Both the src/dst drawing and Poisson inter-arrival time can result in flows competing

**Perfect schedule**
* Source and destination are picked for flow i as i modulo n and (i + 1) modulo n. As such, the only interference is between the ACK stream of i and the data stream of i + 1.
* Flow inter-arrival time is fixed to 1.0 / arrival-rate. E.g., for arrival-rate = 10 flows/s, it means exactly every 0.1s a flow is started.
* Only the ACK/data stream co-location can cause minor competition, or if the arrival-rate is beyond the saturation point.
 

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
   This will generate FCT statistics for each run folder (in `analysis/flows.statistics` of each run folder) and general statistics among them in `data_{random, perfect}_schedule/`
   
3. Execute:
   ```
   bash plot.sh
   ```
   This will use the files in `data_{random, perfect}_schedule/` to create a few plots in `pdf/`
