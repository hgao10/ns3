#ifndef UTILIZATION_TRACKER_H
#define UTILIZATION_TRACKER_H

#include<stdint.h>
#include<vector>
#include<string>

namespace ns3{

class UtilizationTracker{

private:
  bool m_utilization_tracking_enabled = false;
  int64_t m_interval_ns;
  int64_t m_prev_time_ns;
  int64_t m_current_interval_start;
  int64_t m_current_interval_end;
  int64_t m_idle_time_counter_ns;
  int64_t m_busy_time_counter_ns;
  bool m_current_state_is_on;
  std::vector<double> m_utilization;
  
  int64_t m_node_id;
  std::string m_base_logdir;
  void Record(double u);

public:
  void TrackUtilization(bool next_state_is_on);
  void EnableUtilizationTracking(int64_t interval_ns, int64_t node, std::string baselogdir);
  const std::vector<double>& FinalizeUtilization();
};

}

#endif