import matplotlib.pyplot as plt
import sys


def plot(input_file):
    names = []
    measurements = []
    with open(input_file) as fp:
        line = fp.readline()
        names = line.split(",")
        names[-1] = names[-1].strip()
        line = fp.readline()
        while line:
            tokens = line.split(",")
            tokens[-1] = tokens[-1].strip()
            d = {}
            for name, i in zip(names,range(len(names))):
                d[name] = tokens[i]
            measurements.append(d)
            line = fp.readline()

    cpus = []
    numa_nodes = []
    regions = []
    for m in measurements:
        cpu       = m['cpu_node']
        numa_node = m['numa_node']
        region    = m['region']
        if cpu not in cpus:
            cpus.append(cpu)
        if numa_node not in numa_nodes:
            numa_nodes.append(numa_node)
        if region not in regions:
            regions.append(region)

    for cpu in cpus:
        for numa_node in numa_nodes:
            for region in regions:
                measurements_ = [m for m in measurements if m[names[0]] == cpu and m['numa_node'] == numa_node and m['region'] == region]

                x = []
                for m in measurements_:
                    metric = float(m['metric'])
                    x.append(metric)

                plt.figure()
                plt.hist(x, bins=100, density=False, histtype='bar', align='mid', orientation='vertical', rwidth=None, label=None)
                plt.xlabel(measurements_[0]['units'])
                mode = 'latency' if 'lat' in input_file else 'throughput'
                title = 'Histogram of ' + mode + ' measurements\n'
                title += names[0] + ": " + cpu + ', numa_node: ' + numa_node + ', region: ' + region
                plt.title(title)
                plt.savefig("plots/hist_" + mode + "_" + names[0] + "_" + cpu + '_numa_node_' + numa_node + '_region_' + region + ".png", bbox_inches = 'tight')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print ('Usage: python plot_extended.py <log_file>')
        exit()
    plot(sys.argv[1])