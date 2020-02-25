#include "reading-helper.h"


/**
 * Sorter of the schedule by (1) start time and (2) flow identifier.
 *
 * @param   i    Schedule entry i
 * @param   j    Schedule entry j
 *
 * @return True iff i starts earlier than j (or if equal start time: true iff flow id of i less than flow id of j)
*/
bool schedule_sorter(const schedule_entry& i, const schedule_entry& j) {
    if (i.start_time_ns == j.start_time_ns) {
        return i.flow_id < j.flow_id;
    } else {
        return i.start_time_ns < j.start_time_ns;
    }
}


/**
 * Read the schedule.csv into a schedule.
 *
 * @param filename      File name of the schedule.csv
 * @param num_nodes     Total number of nodes present (to check identifiers)
 * @param schedule      Schedule
*/
void read_schedule(const std::string& filename, const int64_t num_nodes, std::vector<schedule_entry>& schedule) {

    // Open file
    std::string line;
    std::ifstream schedule_file(filename);
    if (schedule_file) {

        // Go over each line
        size_t line_counter = 0;
        while (getline(schedule_file, line)) {

            // Split on ,
            std::vector<std::string> comma_split = split_string(line, ",", 7);

            // Fill entry
            schedule_entry entry = {};
            entry.flow_id = parse_positive_int64(comma_split[0]);
            if (entry.flow_id != (int64_t) line_counter) {
                throw std::invalid_argument(format_string("Flow ID is not ascending by one each line (violation: %" PRId64 ")\n", entry.flow_id));
            }
            entry.from_node_id = parse_positive_int64(comma_split[1]);
            entry.to_node_id = parse_positive_int64(comma_split[2]);
            entry.size_byte = parse_positive_int64(comma_split[3]);
            entry.start_time_ns = parse_positive_int64(comma_split[4]);
            entry.additional_parameters = comma_split[5];
            entry.metadata = comma_split[6];

            // Check node ID
            if (entry.from_node_id >= num_nodes) {
                throw std::invalid_argument(format_string("Invalid from node ID: %" PRId64 ".", entry.from_node_id));
            }
            if (entry.to_node_id >= num_nodes) {
                throw std::invalid_argument(format_string("Invalid to node ID: %" PRId64 ".", entry.to_node_id));
            }

            // Put into schedule
            schedule.push_back(entry);

            // Next line
            line_counter++;

        }

        // Close file
        schedule_file.close();

    } else {
        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }

    // Finally sort schedule ascending on time
    std::sort(schedule.begin(), schedule.end(), schedule_sorter);

}
