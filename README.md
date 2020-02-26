# Packet-level simulations

This repository enables the convenient execution of packet-level experiments using the ns-3 simulator. It only takes in the input of a configuration file, which describes the minimal necessary parameters, a topology file, and a flow arrival schedule. It has the ability to enable switch-id-seeded ECMP.

**Make sure you fork this repository and continue on it in your own repository. This repository is kept simple on purpose.**

## Getting started

1. Extract ns-3.30.1.zip (this is given for convenience, this is the generic ns-3):
   ```
   unzip ns-3.30.1.zip
   ```
   
2. Now remove the scratch folder from the ns-3.30.1 folder (where it was extracted to), as we have our own:
   ```
   rm -rf ns-3.30.1/scratch
   ```
   
3. Move the contents to the simulator directory:
   ```
   cp -r ns-3.30.1/* simulator/
   ```
   
4. Remove the ns-3.30.1 folder as we do not need it anymore:
   ```
   rm -r ns-3.30.1
   ```

5. Navigate to simulator:
   ```
   cd simulator
   ```

6. Replace the content of `src/internet/model/ipv4-global-routing.cc / .h` with that of `scratch/main/replace-ipv4-global-routing.cc / .h`:
   ```
   cp scratch/main/replace-ipv4-global-routing.cc.txt src/internet/model/ipv4-global-routing.cc
   cp scratch/main/replace-ipv4-global-routing.h.txt src/internet/model/ipv4-global-routing.h
   ```

7. Make sure you have the latest version of Python (3.7+)
   
8. Configure
    ```bash
    ./waf configure
    ```
   
9. Build
    ```bash
    ./waf
    ```
   
10. Run the example:
    ```bash
    ./waf --run="main --run_dir='../runs/example_single'"
    ```

11. Check out the results:
    ```bash
    xdg-open ../runs/example_single/logs/flows.txt
    ```
 
 ## Speed-up the simulation
 
If you want to speed-up the simulation (disables logging), configuring as follows helps:
 ```bash
 ./waf configure --build-profile=optimized
 ```


## Run folder

The run folder must contain the input of a simulation.

**config.properties**

General properties of the simulation. The following are already defined:

* `filename_topology` : Topology filename (relative to run folder)
* `filename_schedule` : Schedule filename (relative to run folder)
* `simulation_end_time_ns` : How long to run the simulation in simulation time (ns)
* `simulation_seed` : If there is randomness present in the simulation, this guarantees reproducibility (exactly the same outcome) if the seed is the same
* `link_data_rate_megabit_per_`s : Data rate set for all links (Mbit/s)
* `link_delay_ns` : Propagation delay set for all links (ns)

**schedule.csv**

Flow arrival schedule. Each line defines a flow as follows:

```
flow_id,from_node_id,to_node_id,size_byte,start_time_ns,additional_parameters,metadata
```

Notes: flow_id must increment each line. All values except additional_parameters and metadata are mandatory. `additional_parameters` should be set if you want to configure special for each flow (e.g., different transport protocol). `metadata` is for identification later on (e.g., to indicate the workload it is part of).

**topology.properties**

The topological layout of the network. Please see the examples to understand each property. Besides it just defining a graph, the following rules apply:

* If there are servers defined, they can only have edges to a ToR.
* There is no difference with what is installed by main.cc for switches/ToRs/servers: they all get the same stack/hardware.
* The only difference is when interpreting the schedule.csv: (a) if there are servers defined: from/to must be servers, (b) else if there are no servers defined, from/to must be marked as ToRs (it's like a sanity check).


## Some recommendations and notes

Based on some experience with packet-level simulation, I would like to add the following notes:

* **Strong recommendation: keep any run input generation (e.g., config/schedule/topology) separate from the main.cc!** Make a different repository that generates run folders, which is then inputted using main.cc. Mixing up the run folders and the main function will lead to unmaintainable code, and is also unnecessary: the simulator is for simulating, not for generating simulation input. It also makes it more difficult to reproduce old runs. Just put all the generation code into another project repository, in which you use languages like Python which are more effective at this job anyway.

* **Why is my simulation slow?** Discrete packet-level simulation speed (as in wallclock seconds for each simulation second) is chiefly determined by events of packets going being set over links. You can interpret this as follows:
  - The wallclock time it takes to simulate 10 flows going over a single link is about the same as 1 flow going over a link.
  - The wallclock time it takes to simulate a flow going over 5 links (hops) is roughly 5x as slow as a flow going over 1 link (hop).
  - The wallclock time it takes to simulate a flow going over a 10 Mbit/s link for 100s takes about as long as a flow going over a 100 Mbit/s link for 10s.
  - Idle links don't increase wallclock time because there are barely any events happening there (maybe routing updates).

* **How do I implement feature X?** This repository is only a simple way to run a simulation, it is a starting point. You will have to write your own code if you want e.g. different delays or rates for different links, custom transport layer protocols, new queue implementations etc.. All of the things ns-3 has modularized already, and you can just implement and plug in (all of the three examples in the previous sentences can be done quite easily). There are things that are more difficult in ns-3, such as your own custom routing protocol.

* **To maintain reproducibility, any randomness inside your code must be drawn from the ns-3 randomness classes which were initialized by the simulation seed!** Runs must be reproducible in a discrete event simulation run.


## Acknowledgements

Based on code written by Hussain, who did his master thesis in the NDAL group.
Refactored, extended and maintained by Simon. The ECMP extension is adapted from https://github.com/mkheirkhah/ecmp (retrieved February 20th, 2020).
