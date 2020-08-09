from ring_topology_helper import *


def generate_flow_to_tuples_ring_topology_test():
    num_nodes_test_cases = [3, 5]
    results = [[(0,1), (1,2), (2,0)], [(0,1), (1,2), (2,3), (3,4),(4,0)]]
    for i in range(len(num_nodes_test_cases)):
        tuples = generate_flow_to_tuples_ring_topology(num_nodes_test_cases[i])
        assert tuples == results[i], f"generate_flow_to_tuples_ring_topology_test failed for testcase {i}"


def test_leaf_tor_topology_gen_func(test_dir):
    num_nodes = [5,8]
    for n in num_nodes:
        topology_file = test_dir/f"test_leaf_tor_topology_file_{n}"
        generate_leaf_tor_topology(n, topology_file)

def test_ring_topology_gen_func(test_dir):
    num_nodes = [5,8]
    for n in num_nodes:
        topology_file = test_dir/f"test_topology_file_{n}"
        generate_ring_topology_file(n, topology_file)


if __name__== "__main__":
    generate_flow_to_tuples_ring_topology_test()