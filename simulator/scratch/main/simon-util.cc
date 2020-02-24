#include "simon-util.h"

int64_t parse_positive_int64(const std::string& str) {
    int64_t val = std::stoll(str);
    if (val < 0) {
        throw std::invalid_argument(format_string("Negative int64 value not permitted: " PRId64, val));
    }
    return val;
}

int64_t parse_geq_one_int64(const std::string& str) {
    int64_t val = std::stoll(str);
    if (val < 1) {
        throw std::invalid_argument(format_string("Value must be >= 1: " PRId64, val));
    }
    return val;
}

double parse_positive_double(const std::string& str) {
    double val = std::stod(str);
    if (val < 0) {
        throw std::invalid_argument(format_string("Negative double value not permitted: %f", val));
    }
    return val;
}

double parse_double_between_zero_and_one(const std::string& str) {
    double val = std::stod(str);
    if (val < 0 || val > 1) {
        throw std::invalid_argument(format_string("Double value must be [0, 1]: %f", val));
    }
    return val;
}

bool parse_boolean(const std::string& str) {
    if (str == "true" || str == "1") {
        return true;
    } else if (str == "false" || str == "0") {
        return false;
    } else {
        throw std::invalid_argument(format_string("Value could not be converted to bool: %s", str.c_str()));
    }
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::set<int64_t> direct_set_intersection(std::set<int64_t> s1, std::set<int64_t> s2) {
    std::set<int64_t> intersect;
    set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(intersect, intersect.begin()));
    return intersect;
}

void all_items_are_less_than(std::set<int64_t> s, int64_t number) {
    for (int64_t val : s) {
        if (val >= number) {
            throw std::invalid_argument(
                    format_string(
                            "Value %" PRIu64 " > %" PRIu64 "",
                            val,
                            number
                    )
            );
        }
    }
}

std::set<int64_t> direct_set_union(std::set<int64_t> s1, std::set<int64_t> s2) {
    std::set<int64_t> s_union;
    set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(s_union, s_union.begin()));
    return s_union;
}

std::set<int64_t> parse_set_positive_int64(std::string& line) {
    std::set<std::string> string_set = parse_set_string(line);
    std::set<int64_t> int64_set;
    for (std::string s : string_set) {
        int64_set.insert(parse_positive_int64(s));
    }
    if (string_set.size() != int64_set.size()) {
        throw std::invalid_argument(
                format_string(
                        "Set %s contains int64 duplicates",
                        line.c_str()
                )
        );
    }
    return int64_set;
}

std::set<std::string> parse_set_string(std::string& line) {

    // Check set(...)
    if (!starts_with(line, "set(") || !ends_with(line, ")")) {
        throw std::invalid_argument(format_string( "Set %s is not encased in set(...)",line.c_str()));
    }
    std::string only_inside = line.substr(4, line.size() - 5);
    if (trim(only_inside).empty()) {
        std::set<std::string> final;
        return final;
    }
    std::vector<std::string> final = split_string(only_inside, ",");
    std::set<std::string> final_set;
    for (std::string& s : final) {
        final_set.insert(trim(s));
    }
    if (final.size() != final_set.size()) {
        throw std::invalid_argument(
                format_string(
                        "Set %s contains duplicates",
                        line.c_str()
                )
        );
    }
    return final_set;
}

/**
 * Get the additional parameter value or fail.
 *
 * @param param_key                 Parameter key
 * @param additional_parameters     Additional parameters map
 *
 * @return Parameter value (if not present, an invalid_argument exception is thrown)
 */
std::string get_param_or_fail(const std::string& param_key, std::map<std::string, std::string>& additional_parameters) {

    if (additional_parameters.find(param_key) != additional_parameters.end()) {
        return additional_parameters[param_key];
    } else {
        throw std::invalid_argument(format_string("Necessary parameter '%s' is not set.", param_key.c_str()));
    }

}

std::vector<std::string> split_string(std::string& line, const std::string& delimiter) {

    /* Original C++ version, but which did not work for empty values at the start/end:

    // Split by delimiter
    std::regex regex_comma(delimiter);
    std::vector<std::string> the_split{
            std::sregex_token_iterator(line.begin(), line.end(), regex_comma, -1), {}
    };

     */

    // Split by delimiter
    char *cline = (char*) line.c_str();
    char * pch;
    pch = strsep(&cline, delimiter.c_str());
    size_t i = 0;
    std::vector<std::string> the_split;
    while (pch != nullptr) {
        the_split.emplace_back(pch);
        pch = strsep(&cline, delimiter.c_str());
        i++;
    }

    return the_split;
}

std::vector<std::string> split_string(std::string& line, const std::string& delimiter, size_t expected) {

    std::vector<std::string> the_split = split_string(line, delimiter);

    // It must match the expected split length, else throw exception
    if (the_split.size() != expected) {
        throw std::invalid_argument(
                format_string(
                        "String %s has a %s-split of %zu != %zu",
                        line.c_str(),
                        delimiter.c_str(),
                        the_split.size(),
                        expected
                )
        );
    }

    return the_split;
}


/**
 * Trim from end of string.
 *
 * Source: https://stackoverflow.com/a/25385766
 *
 * @param s     Original string
 *
 * @return Right-trimmed string
 */
std::string right_trim(std::string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

/**
 * Trim from start of string.
 *
 * Source: https://stackoverflow.com/a/25385766
 *
 * @param s     Original string
 *
 * @return Left-trimmed string
 */
std::string left_trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

/**
 * Trim from start and end of string.
 *
 * Source: https://stackoverflow.com/a/25385766
 *
 * @param s     Original string
 *
 * @return Left-and-right-trimmed string
 */
std::string trim(std::string& s)
{
    return left_trim(right_trim(s));
}

std::string remove_start_end_double_quote(std::string s) {
    s.erase(std::remove( s.begin(), s.end(), '\"' ), s.end());
    return s;
}

/**
 * Read a config.ini into a mapping.
 *
 * @param   filename    File name of the config.ini
 * @param   config      Key-value map to store the configuration
*/
void read_config(const std::string& filename, std::map<std::string, std::string>& config) {

    // Open file
    std::string line;
    std::ifstream config_file(filename);
    if (config_file) {

        // Go over each line
        while (getline(config_file, line)) {

            // Skip commented lines
            if (trim(line).empty() || line.c_str()[0] == '#') {
                continue;
            }

            // Split on =
            std::vector<std::string> equals_split = split_string(line, "=", 2);

            // Save into config
            config[equals_split[0]] = remove_start_end_double_quote(equals_split[1]);

        }
        config_file.close();
    } else {
        throw std::runtime_error(format_string("File %s could not be read.\n", filename.c_str()));
    }

}

double byte_to_megabit(int64_t num_bytes) {
    return num_bytes * 8.0 / 1000.0 / 1000.0;
}

double nanosec_to_sec(int64_t num_seconds) {
    return num_seconds / 1e9;
}

double nanosec_to_millisec(int64_t num_seconds) {
    return num_seconds / 1e6;
}

double nanosec_to_microsec(int64_t num_seconds) {
    return num_seconds / 1e3;
}

void remove_file_if_exists(std::string filename) {
    struct stat st = {0};
    if (stat(filename.c_str(), &st) == 0) {
        if (unlink(filename.c_str()) == -1) {
            throw std::runtime_error(format_string("File %s could not be removed.\n", filename.c_str()));
        }
    }
}
