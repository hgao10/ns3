import numpy as np
import csv
import os


def analyze():

    for t in [("perfect_schedule", False, [0]), ("random_schedule", True, [0, 1, 2, 3, 4])]:

        # Base values
        arrival_rates = [10, 50, 100, 150, 200, 250, 300, 350, 400]
        seeds = t[2]

        # Where we store the statistics to generate plottable data files
        all_statistics = {}
        for arrival_rate in arrival_rates:
            all_statistics[arrival_rate] = []

        for arrival_rate_flows_per_s in arrival_rates:
            for seed in seeds:

                if t[1]:
                    run_folder_path = "runs/run_" + t[0] + "_s" + str(seed) + "_" + str(arrival_rate_flows_per_s)
                else:
                    run_folder_path = "runs/run_" + t[0] + "_" + str(arrival_rate_flows_per_s)

                time_low_bound_s = 2   # Warm-up is [0, 2)
                time_high_bound_s = 7  # Cool-down is [7, 10)

                time_low_bound_ns = time_low_bound_s * 1000 * 1000 * 1000
                time_high_bound_ns = time_high_bound_s * 1000 * 1000 * 1000

                # Create analysis folder
                analysis_folder_path = run_folder_path + '/analysis'
                if not os.path.exists(analysis_folder_path):
                    os.makedirs(analysis_folder_path)

                # Check that the run is finished
                if os.path.isfile(run_folder_path + '/logs/finished.txt'):
                    with open(run_folder_path + '/logs/finished.txt', 'r') as f:
                        if not f.readline().strip() == "Yes":
                            raise ValueError("Run %s not yet finished." % run_folder_path)
                else:
                    raise ValueError("Run %s not even started." % run_folder_path)

                # Read connection information
                with open(run_folder_path + '/logs/flows.csv') as f:
                    reader = csv.reader(f)

                    # Column lists
                    print("Consuming flows.csv...")

                    # Read in column lists
                    fcts = []
                    num_completed = 0
                    num_dnf = 0
                    num_err = 0
                    for row in reader:
                        if len(row) != 10:
                            raise ValueError("Invalid line: ", row)

                        start_time_ns = int(row[4])
                        if time_low_bound_ns <= start_time_ns < time_high_bound_ns:
                            flow_id = int(row[0])
                            from_node_id = int(row[1])
                            to_node_id = int(row[2])
                            size_byte = int(row[3])
                            start_time_ns = int(row[4])
                            end_time_ns = int(row[5])
                            duration_ns = int(row[6])
                            amount_sent_byte = int(row[7])
                            finished = row[8]
                            metadata = row[9]

                            if finished == "YES":
                                fcts.append(duration_ns)
                                num_completed += 1
                            elif finished == "DNF":
                                num_dnf += 1
                            elif finished == "ERR":
                                num_err += 1
                            else:
                                raise ValueError("Invalid finished value: ", finished)

                    print("Calculating statistics...")

                    statistics = {
                        'num_total': num_completed + num_dnf + num_err,
                        'num_completed': num_completed,
                        'num_dnf': num_dnf,
                        'num_err': num_err,
                        'fct_ns_mean': np.mean(fcts),
                        'fct_ns_median': np.median(fcts),
                        'fct_ns_99th': np.percentile(fcts, 99),
                        'fct_ns_99.9th': np.percentile(fcts, 99.9),
                        'fct_ns_90th': np.percentile(fcts, 90),
                        'fct_ns_1th': np.percentile(fcts, 1),
                        'fct_ns_0.1th': np.percentile(fcts, 0.1),
                        'fct_ns_10th': np.percentile(fcts, 10),
                        'fct_ns_max': np.max(fcts),
                        'fct_ns_min': np.min(fcts),
                    }

                    # Add to general statistics
                    all_statistics[arrival_rate_flows_per_s].append(statistics)

                    # Print raw results
                    statistics_filename = "%s/flows.statistics" % analysis_folder_path
                    print('Writing to result file %s...' % statistics_filename)
                    with open(statistics_filename, 'w+') as outfile:
                        for key, value in sorted(statistics.items()):
                            outfile.write(str(key) + "=" + str(value) + "\n")

        # Data folder
        data_path = "data_" + t[0]
        if not os.path.exists(data_path):
            os.makedirs(data_path)

        # Write the statistics to the plot-able data files
        for stat in ['num_total', 'num_completed', 'num_dnf', 'num_err', 'fct_ns_mean', 'fct_ns_median',
                     'fct_ns_99th', 'fct_ns_99.9th', 'fct_ns_90th', 'fct_ns_1th', 'fct_ns_0.1th',
                     'fct_ns_10th', 'fct_ns_max', 'fct_ns_min']:
            with open(data_path + "/%s.txt" % stat, "w+") as f_out:
                for arrival_rate in arrival_rates:
                    values = []
                    for entry in all_statistics[arrival_rate]:
                        values.append(entry[stat])
                    f_out.write("%d\t%f\t%f\t%d\n" % (
                        arrival_rate,
                        1.0 * np.mean(values),
                        2 * np.std(values, ddof=1) if len(values) > 1 else 0,  # 2 times the sample standard deviation,
                                                                               # which is approximately 95% confidence
                                                                               # according to 68-95-99.7 rule
                        len(values)
                    ))


if __name__ == "__main__":
    analyze()
