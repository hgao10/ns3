import sys
import random
import math


def draw_poisson_inter_arrival_gap(lambda_mean_arrival_rate):
    """
    Draw a poisson inter-arrival gap.
    It uses random as random source, so be sure to set random.seed(...) beforehand for reproducibility.

    E.g.:
    If lambda = 1000, then mean gap is 0.001
    If lambda = 0.1, then mean gap is 10

    :param lambda_mean_arrival_rate:     Lambda mean arrival rate (i.e., every 1 in ... an event arrives)

    :return: Value drawn from the exponential distribution (i.e., Poisson inter-arrival distribution)
    """
    return - math.log(1.0 - random.random(), math.e) / lambda_mean_arrival_rate


def get_schedule_poisson_arrival_uniform(seed, max_duration_ns, poisson_arrival_rate_per_s, node_ids):
    """
    Generate a poisson arrival with uniformly drawing from and to.

    :param seed:                        Randomness seed
    :param max_duration_ns:             Maximum duration in nanoseconds to generate the schedule
    :param poisson_arrival_rate_per_s:  Lambda poisson arrival rate
    :param node_ids:                    Node identifiers

    :return: Array of 3-tuples: [(start_time_ns, from, to), ...]
    """

    if poisson_arrival_rate_per_s == 0:
        return []

    # Random init
    random.seed(seed)

    # Schedule of 3-tuples: (start_time_ns, from_id, to_id)
    schedule = []

    # Fill schedule
    prev_time_ns = 0
    while True:

        # Draw from Poisson process
        inter_arrival_gap_s = draw_poisson_inter_arrival_gap(poisson_arrival_rate_per_s)
        prev_time_ns += inter_arrival_gap_s * 1000 * 1000 * 1000

        # End if we are generating a flow starting after the end
        if prev_time_ns > max_duration_ns:
            break

        # From and to
        from_id = node_ids[random.randint(0, len(node_ids) - 1)]
        to_id = node_ids[random.randint(0, len(node_ids) - 1)]
        while from_id == to_id:
            to_id = node_ids[random.randint(0, len(node_ids) - 1)]

        # Add to schedule
        schedule.append((prev_time_ns, from_id, to_id))

    return schedule


def schedule_generator(
        out_filename,
        arrival_rate_flows_per_s,
        flow_size_byte,
        runtime_ns,
        node_ids,
        seed_arrival,
):

    with open(out_filename, "w+") as f_schedule:

        # Generate schedule
        schedule = get_schedule_poisson_arrival_uniform(
            seed_arrival, runtime_ns, arrival_rate_flows_per_s, node_ids
        )

        # Write to file
        i = 0
        for event in schedule:
            f_schedule.write("%d,%d,%d,%d,%d,%s,%s\n" % (
                i, event[1], event[2], flow_size_byte, event[0], "", ""
            ))
            i += 1


def main():
    args = sys.argv[1:]
    if len(args) != 6:
        print("Usage: python schedule_generator.py"
              " <out_filename> <arrival rate (flows/s)> <flow size (byte)> <runtime (s)> <# of nodes> <seed>")
        print("Example:")
        print("")
        print("               python schedule_generator_random.py schedule.csv 100 100000 10 4 123456789")
        print("")
    else:
        out_filename = args[0]
        arrival_rate_flows_per_s = int(args[1])
        flow_size_byte = int(args[2])
        runtime_ns = int(args[3]) * 1000 * 1000 * 1000
        node_ids = []
        for i in range(int(args[4])):
            node_ids.append(i)
        seed = int(args[5])
        schedule_generator(out_filename, arrival_rate_flows_per_s, flow_size_byte, runtime_ns, node_ids, seed)


if __name__ == "__main__":
    main()
