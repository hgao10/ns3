

def read_pingmesh_csv(logs_ns3_dir):
    rtt_results = {}  # (from, to) -> (is_lost: bool, send_request_timestamp: int, rtt_ns: int)
    with open(logs_ns3_dir + "/pingmesh.csv", "r") as in_file:
        for line in in_file:
            spl = line.split(",")
            from_node_id = int(spl[0])
            to_node_id = int(spl[1])
            i = int(spl[2])
            send_request_timestamp = int(spl[3])
            reply_timestamp = int(spl[4])
            receive_reply_timestamp = int(spl[5])
            latency_to_there_ns = int(spl[6])
            latency_from_there_ns = int(spl[7])
            rtt_ns = int(spl[8])
            if spl[9].strip() == "YES":
                is_lost = False
            elif spl[9].strip() == "LOST":
                is_lost = True
            else:
                raise ValueError("Is lost must be YES or LOST: " + spl[9])

            # Insert into RTTs for the pair
            if not (from_node_id, to_node_id) in rtt_results:
                rtt_results[(from_node_id, to_node_id)] = []
            if len(rtt_results[(from_node_id, to_node_id)]) != i:
                raise ValueError("Invalid entry, i should be ascending")
            rtt_results[(from_node_id, to_node_id)].append((is_lost, send_request_timestamp, rtt_ns))

    return rtt_results
