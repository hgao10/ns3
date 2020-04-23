# Ping plotting

Quick utility to plot the output of UdpRttClient.

**Usage ping plot to plot time vs. RTT:**

```
python ping_plot.py [ping_plot directory] [logs_ns3 directory] [from_id] [to_id]
```

**Usage ping analysis to identify interesting ping pairs:**

```
python pings_analysis.py [logs_ns3 directory]
```

**Dependencies:**

This folder makes use of exputil. Please download it using:

https://gitlab.inf.ethz.ch/OU-SINGLA/exputilpy

... or install directly:

```
pip install git+https://gitlab.inf.ethz.ch/OU-SINGLA/exputilpy.git
```
