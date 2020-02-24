#ifndef SIMON_UTIL_H
#define SIMON_UTIL_H

#include <string>
#include <memory>
#include <stdexcept>
#include <cinttypes>
#include <vector>
#include <set>
#include <regex>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

/**
 * Direct format() for a string in one line (e.g., for exceptions).
 *
 * Licensed under CC0 1.0.
 *
 * Source:
 * https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf/26221725#26221725
 *
 * @tparam Args         Argument types (e.g., char*, int)
 * @param format        String format (e.g., "%s%d")
 * @param args          Argument (e.g., "test", 8)
 *
 * @return Formatted string
 */
template<typename ... Args>
std::string format_string( const std::string& format, Args ... args ) {
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

int64_t parse_positive_int64(const std::string& str);
double parse_positive_double(const std::string& str);
bool parse_boolean(const std::string& str);
double parse_double_between_zero_and_one(const std::string& str);
int64_t parse_geq_one_int64(const std::string& str);
std::set<std::string> parse_set_string(std::string& line);
std::set<int64_t> direct_set_intersection(std::set<int64_t> s1, std::set<int64_t> s2);
std::set<int64_t> direct_set_union(std::set<int64_t> s1, std::set<int64_t> s2);
std::set<int64_t> parse_set_positive_int64(std::string& line);
void all_items_are_less_than(std::set<int64_t> s, int64_t number);
std::vector<std::string> split_string(std::string& line, const std::string& delimiter);
std::vector<std::string> split_string(std::string& line, const std::string& delimiter, size_t expected);
std::string get_param_or_fail(const std::string& param_key, std::map<std::string, std::string>& additional_parameters);
std::string right_trim(std::string s);
std::string left_trim(std::string s);
std::string trim(std::string& s);
std::string remove_start_end_double_quote(std::string s);
void read_config(const std::string& filename, std::map<std::string, std::string>& config);
double byte_to_megabit(int64_t num_bytes);
double nanosec_to_sec(int64_t num_seconds);
double nanosec_to_millisec(int64_t num_seconds);
double nanosec_to_microsec(int64_t num_seconds);
void remove_file_if_exists(std::string filename);

#endif //SIMON_UTIL_H
