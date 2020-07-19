#include "utilization-tracker.h"
#include "ns3/basic-simulation.h"

namespace ns3{

void UtilizationTracker::EnableUtilizationTracking(int64_t interval_ns, int64_t node, std::string baselogdir) {
    m_utilization_tracking_enabled = true;
    m_interval_ns = interval_ns;
    m_prev_time_ns = 0;
    m_current_interval_start = 0;
    m_current_interval_end = m_interval_ns;
    m_idle_time_counter_ns = 0;
    m_busy_time_counter_ns = 0;
    m_current_state_is_on = false;
    m_node_id = node;
    m_base_logdir = baselogdir;
}

void UtilizationTracker::Record(double u){
    std::ofstream ofs;
    ofs.open(m_base_logdir + "/" + format_string("NetworkDevice_%d_utilization.txt", m_node_id),
    std::ofstream::out | std::ofstream::app);
    ofs <<u << " "<< Simulator::Now().GetNanoSeconds()<<std::endl;
    ofs.close(); 
}

void UtilizationTracker::TrackUtilization(bool next_state_is_on) {
    if (m_utilization_tracking_enabled) {
        // Current time in nanoseconds
        int64_t now_ns = Simulator::Now().GetNanoSeconds();
        while (now_ns >= m_current_interval_end) {
            // Add everything until the end of the interval
            if (next_state_is_on) {
                m_idle_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            } else {
                m_busy_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            }
            // Save into the utilization array
            double utilization = ((double) m_busy_time_counter_ns) / ((double) m_interval_ns);
            // m_utilization.push_back(((double) m_busy_time_counter_ns) / ((double) m_interval_ns));
            m_utilization.push_back(utilization);
            Record(utilization);
            // This must match up
            if (m_idle_time_counter_ns + m_busy_time_counter_ns != m_interval_ns) {
                std::cout << m_idle_time_counter_ns << std::endl;
                std::cout << m_busy_time_counter_ns << std::endl;
                throw std::runtime_error("Must match up");
            }
            // Move to next interval
            m_idle_time_counter_ns = 0;
            m_busy_time_counter_ns = 0;
            m_prev_time_ns = m_current_interval_end;
            m_current_interval_start += m_interval_ns;
            m_current_interval_end += m_interval_ns;
        }
        // If not at the end of a new interval, just keep track of it all
        if (next_state_is_on) {
            m_idle_time_counter_ns += now_ns - m_prev_time_ns;
        } else {
            m_busy_time_counter_ns += now_ns - m_prev_time_ns;
        }
        // This has become the previous call
        m_current_state_is_on = next_state_is_on;
        m_prev_time_ns = now_ns;
    }
}

const std::vector<double>& UtilizationTracker::FinalizeUtilization() {
    TrackUtilization(!m_current_state_is_on);
    return m_utilization;
}

}