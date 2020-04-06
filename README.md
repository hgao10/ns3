# Packet-level simulations

This repository enables the convenient execution of packet-level experiments using the ns-3 simulator. It only takes in the input of a configuration file, which describes the minimal necessary parameters, a topology file, and a flow arrival schedule. Default routing applied is switch-seeded 5-tuple ECMP.

**Make sure you fork this repository and continue on it in your own repository. This repository is kept simple on purpose. You can keep your fork up-to-date by pulling it in regularly from your own repository: `git pull git@gitlab.inf.ethz.ch:OU-SINGLA/ns3-basic-sim.git`**

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

6. Make sure you have a recent version of Python (3.6+)
   
7. Configure
    ```
    ./waf configure --enable-tests
    ```
   
8. Build
    ```
    ./waf
    ```
   
9. Run the example:
    ```
    ./waf --run="main --run_dir='../runs/example_single'"
    ```

10. Check out the results:
    ```
    xdg-open ../runs/example_single/logs_ns3/flows.txt
    ```
 
 ## Running tests
 
To run all the tests for the basic simulation:

```
cd simulator
python test.py -v -s "basic-sim" -t test-results
xdg-open test-results.txt
```
 
 ## Speed-up the simulation
 
If you want to speed-up the simulation (disables logging), configuring as follows helps:
 ```
 ./waf configure --build-profile=optimized
 ```


## Run folder

The run folder must contain the input of a simulation.

**config_ns3.properties**

General properties of the simulation. The following are always defined:

* `filename_topology` : Topology filename (relative to run folder)
* `filename_schedule` : Schedule filename (relative to run folder)
* `simulation_end_time_ns` : How long to run the simulation in simulation time (ns)
* `simulation_seed` : If there is randomness present in the simulation, this guarantees reproducibility (exactly the same outcome) if the seed is the same
* `link_data_rate_megabit_per_s` : Data rate set for all links (Mbit/s)
* `link_delay_ns` : Propagation delay set for all links (ns)

**schedule.csv**

Flow arrival schedule. Each line defines a flow as follows:

```
flow_id,from_node_id,to_node_id,size_byte,start_time_ns,additional_parameters,metadata
```

Notes: flow_id must increment each line. All values except additional_parameters and metadata are mandatory. `additional_parameters` should be set if you want to configure something special for each flow in main.cc (e.g., different transport protocol). `metadata` you can use for identification later on in the flows.csv/txt logs (e.g., to indicate the workload or coflow it was part of).

**topology.properties**

The topological layout of the network. Please see the examples to understand each property. Besides it just defining a graph, the following rules apply:

* If there are servers defined, they can only have edges to a ToR.
* There is no difference with what is installed by main.cc for switches/ToRs/servers: they all get the same stack/hardware.
* The only difference is when interpreting the schedule.csv: (a) if there are servers defined: from/to must be servers, (b) else if there are no servers defined, from/to must be marked as ToRs (it's like a sanity check).

**The log files**

There are three log files generated by the run in the `logs` folder within the run folder:

* `finished.txt` : Contains "Yes" if the run has finished, "No" if not.
* `flows.txt` : Flow results in a visually appealing table.
* `flows.csv` : Flow results in CSV format for processing with each line:

   ```
   flow_id,from_node_id,to_node_id,size_byte,start_time_ns,end_time_ns,duration_ns,amount_sent_byte,[finished: YES/CONN_FAIL/NO_BAD_CLOSE/NO_ERR_CLOSE/NO_ONGOING],metadata
   ```

   (with `YES` = all data was sent and acknowledged fully and there was a normal socket close, `NO_CONN_FAIL` = connection failed (happens only in a very rare set of nearly impossible-to-reach state, typically `NO_BAD_CLOSE` is the outcome when something funky went down with the 3-way handshake), `NO_BAD_CLOSE` = socket closed normally but not all data was transferred (e.g., due to connection timeout), `NO_ERR_CLOSE` = socket error closed, `NO_ONGOING` = socket is still sending/receiving and is not yet closed)

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
Refactored, extended and maintained by Simon. The ECMP routing hashing function is inspired by https://github.com/mkheirkhah/ecmp (retrieved February 20th, 2020).
