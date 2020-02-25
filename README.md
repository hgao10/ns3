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
 
 ## Notes
 
If you want to speed-up the simulation, this helps:
 ```bash
 ./waf configure --build-profile=optimized
 ```

## Acknowledgements

Based on code written by Hussain, who did his master thesis in the NDAL group.
Refactored, extended and maintained by Simon. The ECMP extension is adapted from https://github.com/mkheirkhah/ecmp (retrieved February 20th, 2020).
