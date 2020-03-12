import sys


def get_schedule_perfect(max_duration_ns, arrival_rate_flows_per_s, node_ids):
    """
    Generate a perfectly timed arrival schedule.

    :param max_duration_ns:             Maximum duration in nanoseconds to generate the schedule
    :param arrival_rate_flows_per_s:    Arrival rate
    :param node_ids:                    Node identifiers

    :return: Array of 3-tuples: [(start_time_ns, from, to), ...]
    """

    if arrival_rate_flows_per_s == 0:
        return []

    # Schedule of 3-tuples: (start_time_ns, from_id, to_id)
    schedule = []

    # In case there is perfect from-to
    current = 0

    # Fill schedule
    prev_time_ns = 0
    while True:

        # Draw from Poisson process
        inter_arrival_gap_s = 1.0 / arrival_rate_flows_per_s
        prev_time_ns += inter_arrival_gap_s * 1000 * 1000 * 1000

        # End if we are generating a flow starting after the end
        if prev_time_ns >= max_duration_ns:
            break

        # From and to
        from_id = node_ids[current % len(node_ids)]
        to_id = node_ids[(current + 1) % len(node_ids)]
        current += 1

        # Add to schedule
        schedule.append((prev_time_ns, from_id, to_id))

    return schedule


def schedule_generator(
    out_filename,
    arrival_rate_flows_per_s,
    flow_size_byte,
    runtime_ns,
    node_ids
):
    with open(out_filename, "w+") as f_schedule:
        schedule = get_schedule_perfect(runtime_ns, arrival_rate_flows_per_s, node_ids)
        i = 0
        for event in schedule:
            f_schedule.write("%d,%d,%d,%d,%d,%s,%s\n" % (i, event[1], event[2], flow_size_byte, event[0], "", ""))
            i += 1


def main():
    args = sys.argv[1:]
    if len(args) != 5:
        print("Usage: python schedule_generator.py"
              " <filename> <arrival rate (flows/s)> <flow size (byte)> <runtime (s)> <# of nodes>")
        print("Example:")
        print("")
        print("               python schedule_generator_perfect.py schedule.csv 100 100000 10 4")
        print("")
    else:
        out_filename = args[0]
        arrival_rate_flows_per_s = int(args[1])
        flow_size_byte = int(args[2])
        runtime_ns = int(args[3]) * 1000 * 1000 * 1000
        node_ids = []
        for i in range(int(args[4])):
            node_ids.append(i)
        schedule_generator(out_filename, arrival_rate_flows_per_s, flow_size_byte, runtime_ns, node_ids)


if __name__ == "__main__":
    main()
