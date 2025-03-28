import os
import sys
import termios
import tty
from math import sqrt

import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

folder_path = "_networks/"
protocols = ["aodv", "aodvv2", "aodvv2-multi"]
packet_types = ["RREQ", "RREP", "RERR", "RREP_ACK"]
colors = [
    "#e60049",
    "#0bb4ff",
    "#50e991",
    "#e6d800",
    "#9b19f5",
    "#ffa300",
    "#dc0ab4",
    "#b3d4ff",
    "#00bfa0",
]

output_path = "output-{protocol}.csv"
output_header = "initial battery,final battery,energy consumed\n"

input_ping_path = "output-ping.csv"
output_ping_path = "output-{protocol}-ping.csv"
output_ping_header = "src_ip,dst_ip,hops,first_rtt,min_rtt,max_rtt,avg_rtt\n"

input_route_path = "output-route.csv"

input_rem_nodes_path = "output-rem-nodes.csv"
output_net_life_path = "output-{protocol}-net-life.csv"

input_packets_path = "output-packets.csv"
output_packets_path = "output-{protocol}-packets.csv"
output_packets_header = "type,count,avg,min,max\n"


if not os.path.exists(folder_path):
    os.makedirs(folder_path)


def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch


def clear_console():
    os.system("cls" if os.name == "nt" else "clear")


def menu():
    choice_map = {
        "1": create_networks,
        "2": run_networks,
        "3": get_output_stats,
        "4": save_first_rtt,
        "q": save_aodvv2_uml,
        "w": plot_rtt_comparison,
        "e": save_throughput_comparison,
        "r": save_battery_comparison,
        "t": save_network_lifetime_comparison,
        "0": clean_folder,
    }

    while True:
        clear_console()
        print("\033[1mUTILS\033[0m")
        print("1: Create networks")
        print("2: Run networks")
        print("3: Get output stats")
        print("------------------------------------")
        print("4: Run RTT comparison")

        print("\n\033[1mGRAPHS\033[0m")
        print("q: Save aodvv2 uml")
        print("------------------------------------")
        print("w: Save RTT comparison")
        print("e: Save packet rate comparison")
        print("r: Save battery comparison")
        print("t: Save network lifetime comparison")

        print("\n0: Clean folder")
        print("ESC: Exit")

        choice = getch()

        if choice in choice_map:
            choice_map[choice]()
        else:
            break

        print("\nPress any key to continue...")
        getch()


def create_networks(useInputSetup=False, numNodes=10, numNetworks=5):
    os.system(
        "./ns3 run src/aodvv2/examples/aodvv2-create-networks.cc -- --useInputSetup="
        + str(useInputSetup)
        + " --folderName="
        + folder_path
        + " --numNodes="
        + str(numNodes)
        + " --numNetworks="
        + str(numNetworks)
    )
    print("\nNetworks created!")


def run_networks():
    if not [f for f in os.listdir(folder_path) if f.endswith(".csv")]:
        print("No networks to run!")
        return

    for protocol in protocols:
        if os.path.exists(output_path.replace("{protocol}", protocol)):
            os.remove(output_path.replace("{protocol}", protocol))
        with open(output_path.replace("{protocol}", protocol), "w") as file:
            file.write(output_header)

        if os.path.exists(output_ping_path.replace("{protocol}", protocol)):
            os.remove(output_ping_path.replace("{protocol}", protocol))
        with open(output_ping_path.replace("{protocol}", protocol), "w") as file:
            file.write(output_ping_header)

        if os.path.exists(output_packets_path.replace("{protocol}", protocol)):
            os.remove(output_packets_path.replace("{protocol}", protocol))
        with open(output_packets_path.replace("{protocol}", protocol), "w") as file:
            file.write(output_packets_header)

    for index, file in enumerate([f for f in os.listdir(folder_path) if f.endswith(".csv")]):
        file_path = os.path.join(folder_path, file)
        with open(file_path, "r") as file:
            lines = file.readlines()

        n_nodes = len(lines) - 1
        ping_pairs = get_pairs(index)

        if ping_pairs == "":
            continue

        for protocol in protocols:
            os.system(
                "./ns3 run src/aodvv2/examples/aodvv2-multi-example.cc -- --pcap='false' --topologyFile="
                + file_path
                + " --resultFile="
                + output_path.replace("{protocol}", protocol)
                + " --routingProtocol="
                + protocol
                + " --pingPairs="
                + ping_pairs
                + " --time=10"
            )

            save_stats(protocol)

            with open(output_path.replace("{protocol}", protocol), "r") as file:
                lines = file.readlines()
                if len(lines) - 1 < (index + 1) * n_nodes:
                    with open(output_path.replace("{protocol}", protocol), "a") as file:
                        for i in range(((index + 1) * n_nodes) - len(lines) + 1):
                            file.write("0,0,0\n")

    print("\nResults obtained!")


def get_pairs(index):
    with open(folder_path + "pairs.txt", "r") as file:
        lines = file.readlines()
        return lines[index].strip()


def get_real_pairs(protocol, index):
    with open(output_ping_path.replace("{protocol}", protocol), "r") as file:
        lines = file.readlines()[1:]
        currentIndex = 0
        start = 0
        for i, line in enumerate(lines):
            if line == "-,-,-,-,-,-,-\n":
                if currentIndex == index:
                    return [l.strip() for l in lines[start:i]]
                currentIndex += 1
                start = i + 1

        return []


def get_output_stats():
    files = [f for f in os.listdir(folder_path) if f.endswith(".csv")]
    file_contents = {}

    for i, file in enumerate(files):
        with open(folder_path + file, "r") as file:
            file_contents[i] = {"nodes": [line.strip().split(",") for line in file.readlines()[1:]]}

            for protocol in protocols:
                file_contents[i][protocol + "-ping_pairs"] = get_real_pairs(protocol, i)

                with open(output_path.replace("{protocol}", protocol), "r") as file:
                    file_contents[i][protocol] = [
                        line.strip().split(",") for line in file.readlines()[1:]
                    ]

                with open(output_packets_path.replace("{protocol}", protocol), "r") as file:
                    file_contents[i][protocol + "-packets"] = [
                        line.strip().split(",") for line in file.readlines()[1:]
                    ]

    for i in range(len(file_contents)):
        n_nodes = len(file_contents[i]["nodes"])
        for j in range(int(i * n_nodes), int((i + 1) * n_nodes)):
            if float(file_contents[i][protocol][j][0]) == 0:
                file_contents[i] = {}
                break

    results = {
        protocol: {
            "initial_battery": [],
            "final_battery": [],
            "variance_final_battery": [],
            "energy_consumed": [],
            "hops": [],
            "first_rtt": [],
            "min_rtt": [],
            "max_rtt": [],
            "avg_rtt": [],
            "variance_first_rtt": [],
            "variance_rtt": [],
            "avg_packets": {key: {} for key in packet_types},
        }
        for protocol in protocols
    }
    protocols_index = {protocol: 0 for i, protocol in enumerate(protocols)}

    for i in range(len(file_contents)):
        if file_contents[i] == {}:
            for protocol in protocols:
                protocols_index[protocol] += 1
            continue

        n_nodes = len(file_contents[i]["nodes"])
        n_packets = len(packet_types)

        skip_network = False
        for protocol in protocols:
            n_pairs = len(file_contents[i][protocol + "-ping_pairs"])
            if n_pairs == 0:
                skip_network = True

        if skip_network:
            # print(f"\nNetwork {i + 1}: no pairs found!")
            for protocol in protocols:
                protocols_index[protocol] += 1
            continue

        for protocol in protocols:
            n_pairs = len(file_contents[i][protocol + "-ping_pairs"])
            initial_battery = []
            final_battery = []
            energy_consumed = []
            hops = 0
            first_rtt = 0
            min_rtt = []
            max_rtt = []
            avg_rtt = []
            avg_packets = {}

            for j in range(int(i * n_nodes), int((i + 1) * n_nodes)):
                line = file_contents[i][protocol][j]
                initial_battery.append(float(line[0]))
                final_battery.append(float(line[1]))
                energy_consumed.append(float(line[2]))

            if len(initial_battery) > 0:
                for j in range(n_pairs):
                    line = file_contents[i][protocol + "-ping_pairs"][j].split(",")
                    hops += int(line[2])
                    first_rtt += float(line[3])
                    min_rtt.append(float(line[4]))
                    max_rtt.append(float(line[5]))
                    avg_rtt.append(float(line[6]))

                for j in range(
                    int(protocols_index[protocol] * n_packets),
                    int((protocols_index[protocol] + 1) * n_packets),
                ):
                    line = file_contents[protocols_index[protocol]][protocol + "-packets"][j]
                    if not line[0] in avg_packets:
                        avg_packets[line[0]] = {
                            "count": [],
                            "avg": [],
                            "min": [],
                            "max": [],
                        }
                    if (
                        int(line[1]) == 1
                        and float(line[2]) == 0
                        and int(line[3]) == 0
                        and int(line[4]) == 0
                    ):
                        continue
                    avg_packets[line[0]]["count"].append(int(line[1]))
                    avg_packets[line[0]]["avg"].append(float(line[2]))
                    avg_packets[line[0]]["min"].append(float(line[3]))
                    avg_packets[line[0]]["max"].append(float(line[4]))

            results[protocol]["initial_battery"] = (
                results[protocol].get("initial_battery", []) + initial_battery
            )
            results[protocol]["final_battery"] = (
                results[protocol].get("final_battery", []) + final_battery
            )
            results[protocol]["variance_final_battery"].append(
                round(max(final_battery) - min(final_battery), 3)
            )
            results[protocol]["energy_consumed"] = (
                results[protocol].get("energy_consumed", []) + energy_consumed
            )
            results[protocol]["hops"].append(round(hops / n_pairs, 3))
            results[protocol]["first_rtt"].append(round(first_rtt / n_pairs, 3))
            results[protocol]["min_rtt"].append(round(min(min_rtt), 3))
            results[protocol]["max_rtt"].append(round(max(max_rtt), 3))
            avg_rtt_value = round(sum(avg_rtt) / n_pairs, 3) if n_pairs > 0 else 0
            results[protocol]["avg_rtt"].append(avg_rtt_value)
            mean_rtt = sum(avg_rtt) / len(avg_rtt) if avg_rtt else 0
            var_rtt = sum((r - mean_rtt) ** 2 for r in avg_rtt) / len(avg_rtt) if avg_rtt else 0
            results[protocol]["variance_rtt"].append(round(var_rtt, 3))
            mean_first = (
                sum(results[protocol]["first_rtt"]) / len(results[protocol]["first_rtt"])
                if results[protocol]["first_rtt"]
                else 0
            )
            var_first = (
                sum((fr - mean_first) ** 2 for fr in results[protocol]["first_rtt"])
                / len(results[protocol]["first_rtt"])
                if results[protocol]["first_rtt"]
                else 0
            )
            results[protocol]["variance_first_rtt"].append(round(var_first, 3))
            results[protocol]["avg_packets"] = {
                key: {
                    "count": results[protocol]["avg_packets"].get(key, {}).get("count", [])
                    + avg_packets[key]["count"],
                    "avg": results[protocol]["avg_packets"].get(key, {}).get("avg", [])
                    + avg_packets[key]["avg"],
                    "min": results[protocol]["avg_packets"].get(key, {}).get("min", [])
                    + avg_packets[key]["min"],
                    "max": results[protocol]["avg_packets"].get(key, {}).get("max", [])
                    + avg_packets[key]["max"],
                }
                for key in avg_packets
            }

            if len(initial_battery) > 0:
                protocols_index[protocol] += 1

    output = {}
    valid_networks = len([fc for fc in file_contents.values() if fc != {}])
    print(f"\n\n\033[1mAVERAGE RESULTS (nodes: {n_nodes}, networks: {valid_networks}):\033[0m")
    for protocol in protocols:
        print(f"\n\033[1m{protocol.upper()}\033[0m")
        print(f"{'Metric':<20} | {'Value':<20} | {'Range (min, max)':<20}")
        print("-" * 65)
        print(
            f"{'Initial battery':<20} | {round(sum(results[protocol]['initial_battery']) / len(results[protocol]['initial_battery']), 3):<20} |"
            + f" ({min(results[protocol]['initial_battery'])}, {max(results[protocol]['initial_battery'])})"
        )
        print(
            f"{'Final battery':<20} | {round(sum(results[protocol]['final_battery']) / len(results[protocol]['final_battery']), 3):<20} |"
            + f" ({min(results[protocol]['final_battery'])}, {max(results[protocol]['final_battery'])})"
        )
        print(
            f"{'Battery variance':<20} | {round(sum(results[protocol]['variance_final_battery']) / len(results[protocol]['variance_final_battery']), 3):<20} |"
            + f" ({min(results[protocol]['variance_final_battery'])}, {max(results[protocol]['variance_final_battery'])})"
        )
        print(
            f"{'Energy consumed':<20} | {round(sum(results[protocol]['energy_consumed']) / len(results[protocol]['energy_consumed']), 3):<20} |"
            + f" ({min(results[protocol]['energy_consumed'])}, {max(results[protocol]['energy_consumed'])})"
        )
        print(
            f"{'First RTT':<20} | {round(sum(results[protocol]['first_rtt']) / len(results[protocol]['first_rtt']), 3):<20} |"
            + f" ({min(results[protocol]['first_rtt'])}, {max(results[protocol]['first_rtt'])})"
        )
        print(
            f"{'Avg RTT':<20} | {round(sum(results[protocol]['avg_rtt']) / len(results[protocol]['avg_rtt']), 3):<20} |"
            + f" ({min(results[protocol]['avg_rtt'])}, {max(results[protocol]['avg_rtt'])})"
        )
        print(
            f"{'Variance first RTT':<20} | "
            + f"{round(sum(results[protocol]['variance_first_rtt']) / len(results[protocol]['variance_first_rtt']), 3):<20} |"
            + f" ({min(results[protocol]['variance_first_rtt'])}, {max(results[protocol]['variance_first_rtt'])})"
        )
        print(
            f"{'Variance RTT':<20} | {round(sum(results[protocol]['variance_rtt']) / len(results[protocol]['variance_rtt']), 3):<20} |"
            + f" ({min(results[protocol]['variance_rtt'])}, {max(results[protocol]['variance_rtt'])})"
        )
        print(
            f"{'Avg hops':<20} | {round(sum(results[protocol]['hops']) / len(results[protocol]['hops']), 3):<20} |"
            + f" ({min(results[protocol]['hops'])}, {max(results[protocol]['hops'])})"
        )
        output[protocol] = {
            "first_rtt": round(
                sum(results[protocol]["first_rtt"]) / len(results[protocol]["first_rtt"]), 3
            ),
            "max_rtt": max(results[protocol]["max_rtt"]),
            "min_rtt": min(results[protocol]["min_rtt"]),
            "avg_rtt": round(
                sum(results[protocol]["avg_rtt"]) / len(results[protocol]["avg_rtt"]), 3
            ),
            "variance_first_rtt": round(
                sum(results[protocol]["variance_first_rtt"])
                / len(results[protocol]["variance_first_rtt"]),
                3,
            ),
            "variance_rtt": round(
                sum(results[protocol]["variance_rtt"]) / len(results[protocol]["variance_rtt"]), 3
            ),
        }
        output[protocol]["variance_first_rtt"] = compute_confidence_interval(
            float(output[protocol]["first_rtt"]),
            float(output[protocol]["variance_first_rtt"]),
            valid_networks,
        )
        output[protocol]["variance_rtt"] = compute_confidence_interval(
            float(output[protocol]["avg_rtt"]),
            float(output[protocol]["variance_rtt"]),
            valid_networks,
        )

    print(f"\n\n\033[1mPACKETS:\033[0m")
    for protocol in protocols:
        print(f"\n\033[1m{protocol.upper()}\033[0m")
        print(f"{'Key':<10} | {'Count':<10} | {'Avg':<10} | {'Min':<10} | {'Max':<10}")
        print("-" * 55)
        for key in packet_types:
            if len(results[protocol]["avg_packets"][key]["count"]) == 0:
                print(f"{key:<10} | {'-':<10} | {'-':<10} | {'-':<10} | {'-':<10}")
                continue

            count = round(
                sum(results[protocol]["avg_packets"][key]["count"])
                / len(results[protocol]["avg_packets"][key]["count"]),
                3,
            )
            avg = round(
                sum(results[protocol]["avg_packets"][key]["avg"])
                / len(results[protocol]["avg_packets"][key]["avg"]),
                3,
            )
            min_val = min(results[protocol]["avg_packets"][key]["min"])
            max_val = max(results[protocol]["avg_packets"][key]["max"])
            print(f"{key:<10} | {count:<10} | {avg:<10} | {min_val:<10} | {max_val:<10}")

    return output


def compute_confidence_interval(mean, variance, count, z=1.96):
    if count < 2:
        return (mean, mean)
    std_error = sqrt(variance / count)
    margin = z * std_error
    return (round(mean - margin, 3), round(mean + margin, 3))


def save_first_rtt():
    n_networks = 50
    n_nodes = [5, 10, 20, 30, 40, 50]

    with open("output-run.csv", "w") as csvfile:
        csvfile.write(
            "n_node,protocol,first_rtt,min_rtt,max_rtt,avg_rtt,var_frtt_low,var_frtt_high,var_rtt_low,var_rtt_high\n"
        )

    for n in n_nodes:
        clean_folder()
        create_networks(True, n, n_networks)
        run_networks()
        output = get_output_stats()

        for protocol in protocols:
            first_rtt = output[protocol]["first_rtt"]
            max_rtt = output[protocol]["max_rtt"]
            min_rtt = output[protocol]["min_rtt"]
            avg_rtt = output[protocol]["avg_rtt"]
            variance_first_rtt = output[protocol]["variance_first_rtt"]
            variance_rtt = output[protocol]["variance_rtt"]
            with open("output-run.csv", "a") as csvfile:
                csvfile.write(
                    f"{n},{protocol},{first_rtt},{min_rtt},{max_rtt},{avg_rtt},{variance_first_rtt[0]},{variance_first_rtt[1]},{variance_rtt[0]},{variance_rtt[1]}\n"
                )


def save_battery_comparison():
    size = 4
    remaining_nodes_map = {x: {} for x in range(size)}

    for i, protocol in enumerate(protocols):
        os.system(
            "./ns3 run src/aodvv2/examples/aodvv2-battery-example.cc -- --pcap='false' --routingProtocol="
            + protocol
            + " --size="
            + str(size)
            + " --time=1000"
        )

        index = 0
        with open(input_rem_nodes_path, "r") as file:
            lines = file.readlines()[1:]
            for i, line in enumerate(lines):
                if line == "-,-\n":
                    index = 0
                else:
                    current_battery, throughput = line.strip().split(",")
                    if protocol not in remaining_nodes_map[index]:
                        remaining_nodes_map[index][protocol] = {}
                    if "current_battery" not in remaining_nodes_map[index][protocol]:
                        remaining_nodes_map[index][protocol]["current_battery"] = []
                        remaining_nodes_map[index][protocol]["throughput"] = []
                    remaining_nodes_map[index][protocol]["current_battery"].append(
                        float(current_battery)
                    )
                    remaining_nodes_map[index][protocol]["throughput"].append(
                        float(throughput) / 1000
                    )
                    index += 1

    num_nodes = len(remaining_nodes_map)
    n_rows = 2
    fig, axs = plt.subplots(n_rows, num_nodes, figsize=(num_nodes * 6, n_rows * 6))

    min_last_battery = min(
        [
            remaining_nodes_map[n][protocol]["current_battery"][-1]
            for n in remaining_nodes_map
            for protocol in protocols
        ]
    )
    for i, node in enumerate(remaining_nodes_map):
        ax = axs[0, i]
        for j, protocol in enumerate(protocols):
            if protocol in remaining_nodes_map[node]:
                ax.plot(
                    range(len(remaining_nodes_map[node][protocol]["current_battery"])),
                    remaining_nodes_map[node][protocol]["current_battery"],
                    label=protocol,
                    color=colors[j],
                )
        ax.set_title(f"Node {node} - Remaining battery", weight="bold")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Battery Level (J)")
        ax.set_ylim(min_last_battery - 1, 204.5)
        ax.legend()
        ax.grid(True)

        ax = axs[1, i]

        for j, protocol in enumerate(protocols):
            if protocol in remaining_nodes_map[node]:
                nonzero_x = [
                    x
                    for x, thr in enumerate(remaining_nodes_map[node][protocol]["throughput"])
                    if thr != 0
                ]
                nonzero_data = [
                    thr for thr in remaining_nodes_map[node][protocol]["throughput"] if thr != 0
                ]
                ax.scatter(
                    nonzero_x,
                    nonzero_data,
                    label=protocol,
                    color=colors[j],
                )
                ax.plot(
                    nonzero_x,
                    nonzero_data,
                    color=colors[j],
                )

        ax.set_title(f"Node {node} - Throughput", weight="bold")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Throughput (Kbps)")
        ax.set_ylim(0, 0.9)
        ax.legend()
        ax.grid(True)

    plt.tight_layout()
    plt.savefig(f"output-remaining_battery.pdf")
    plt.close()


def save_network_lifetime_comparison():
    size = 4

    for i, protocol in enumerate(protocols):
        os.system(
            "./ns3 run src/aodvv2/examples/aodvv2-lifetime-example.cc -- --pcap='false' --routingProtocol="
            + protocol
            + " --size="
            + str(size)
            + " --time=5000"
        )

    plot_remaining_nodes(4)


def save_throughput_comparison():
    for protocol in protocols:
        os.system(
            f"./ns3 run examples/routing/manet-routing-compare.cc -- --CSVfileName=output-{protocol}-throughput.csv --protocol="
            + protocol.upper()
        )

    plot_throughput_comparison()


def plot_rtt_comparison():
    results = {}
    with open("output-run.csv", "r") as file:
        lines = file.readlines()[1:]
        for line in lines:
            (
                n_node,
                protocol,
                first_rtt,
                min_rtt,
                max_rtt,
                avg_rtt,
                var_frtt_low,
                var_frtt_high,
                var_rtt_low,
                var_rtt_high,
            ) = line.strip().split(",")
            if n_node not in results:
                results[n_node] = {}
            results[n_node][protocol] = {
                "first_rtt": float(first_rtt),
                "avg_rtt": float(avg_rtt),
                "frtt_low": float(var_frtt_low),
                "frtt_high": float(var_frtt_high),
                "rtt_low": float(var_rtt_low),
                "rtt_high": float(var_rtt_high),
            }

    fig, axs = plt.subplots(2, 1, figsize=(10, 10))

    n_nodes = [int(n) for n in results.keys()]
    first_rtt_results = {protocol: [] for protocol in protocols}
    avg_rtt_results = {protocol: [] for protocol in protocols}
    first_rtt_err_low = {protocol: [] for protocol in protocols}
    first_rtt_err_high = {protocol: [] for protocol in protocols}
    avg_rtt_err_low = {protocol: [] for protocol in protocols}
    avg_rtt_err_high = {protocol: [] for protocol in protocols}

    for n_node in n_nodes:
        for protocol in protocols:
            frtt = results[str(n_node)][protocol]["first_rtt"]
            artt = results[str(n_node)][protocol]["avg_rtt"]
            frtt_low = results[str(n_node)][protocol]["frtt_low"]
            frtt_high = results[str(n_node)][protocol]["frtt_high"]
            rtt_low = results[str(n_node)][protocol]["rtt_low"]
            rtt_high = results[str(n_node)][protocol]["rtt_high"]
            first_rtt_results[protocol].append(frtt)
            avg_rtt_results[protocol].append(artt)
            first_rtt_err_low[protocol].append(max(0, frtt - frtt_low))
            first_rtt_err_high[protocol].append(max(0, frtt_high - frtt))
            avg_rtt_err_low[protocol].append(max(0, artt - rtt_low))
            avg_rtt_err_high[protocol].append(max(0, rtt_high - artt))

    offset = 0.1  # Offset for error bars to avoid overlap
    for i, protocol in enumerate(protocols):
        # axs[0].plot(n_nodes, first_rtt_results[protocol], label=protocol, color=colors[i])
        # axs[1].plot(n_nodes, avg_rtt_results[protocol], label=protocol, color=colors[i])

        x_values = [n + (i - 1) * offset for n in n_nodes]
        axs[0].errorbar(
            x_values,
            first_rtt_results[protocol],
            yerr=[first_rtt_err_low[protocol], first_rtt_err_high[protocol]],
            label=protocol,
            color=colors[i],
            capsize=3,
        )
        axs[1].errorbar(
            x_values,
            avg_rtt_results[protocol],
            yerr=[avg_rtt_err_low[protocol], avg_rtt_err_high[protocol]],
            label=protocol,
            color=colors[i],
            capsize=3,
        )

    # axs[0].set_title("Route Establishment Time", weight="bold")
    axs[0].set_xlabel("Number of Nodes (#)", fontsize=16)
    axs[0].set_ylabel("Route Est Time (ms)", fontsize=16)
    axs[0].set_ylim(-50, 1450)
    axs[0].legend(fontsize=16)
    axs[0].grid(True)

    # axs[1].set_title("Average RTT Comparison", weight="bold")
    axs[1].set_xlabel("Number of Nodes (#)", fontsize=16)
    axs[1].set_ylabel("Average RTT (ms)", fontsize=16)
    axs[1].set_ylim(-50, 1450)
    axs[1].legend(fontsize=16)
    axs[1].grid(True)

    plt.tight_layout()
    plt.savefig("output-rtt_comparison.pdf")
    plt.close()


def plot_throughput_comparison():
    num_cols = 3
    num_rows = (len(protocols) + num_cols - 1) // num_cols
    fig = plt.figure(figsize=(num_cols * 6, num_rows * 6))
    spec = gridspec.GridSpec(ncols=12, nrows=num_rows, figure=fig)
    indices = []

    for i in range(num_rows - 1):
        indices.extend([slice(0, 4), slice(4, 8), slice(8, 12)])
    if len(protocols) % 3 == 0:
        indices.extend([slice(0, 4), slice(4, 8), slice(8, 12)])
    elif len(protocols) % 3 == 1:
        indices.append(slice(4, 8))
    else:
        indices.extend([slice(2, 6), slice(6, 10)])

    all_receive_rates = []

    for protocol in protocols:
        file_path = f"output-{protocol}-throughput.csv"
        with open(file_path, "r") as file:
            lines = file.readlines()[80:]
            for line in lines:
                _, receive_rate, *_ = line.strip().split(",")
                all_receive_rates.append(float(receive_rate))

    min_rate = min(all_receive_rates)
    max_rate = max(all_receive_rates) + 5

    for i, protocol in enumerate(protocols):
        file_path = f"output-{protocol}-throughput.csv"
        times = []
        receive_rates = []
        with open(file_path, "r") as file:
            lines = file.readlines()[80:]
            for line in lines:
                time, receive_rate, *_ = line.strip().split(",")
                times.append(float(time))
                receive_rates.append(float(receive_rate))

        plt.subplot(spec[i // num_cols, indices[i]])
        plt.plot(times, receive_rates, color=colors[i])
        plt.title(protocol, weight="bold")
        plt.xlabel("Simulation Time (s)")
        plt.ylabel("Packet receive Rate (#)")
        plt.ylim(min_rate, max_rate)
        plt.grid(True)

    plt.tight_layout()
    plt.savefig("output-throughput_comparison.pdf")
    plt.close()


def plot_remaining_nodes(initial_nodes=None):
    avg_nodes = {protocol: {} for protocol in protocols}
    index = 0

    for protocol in protocols:
        with open(output_net_life_path.replace("{protocol}", protocol), "r") as file:
            lines = file.readlines()[1:]
            for i, line in enumerate(lines):
                if line == "-,-\n":
                    index = 0
                else:
                    time, count = line.strip().split(",")
                    if initial_nodes is None:
                        initial_nodes = count
                    if index not in avg_nodes[protocol]:
                        avg_nodes[protocol][index] = []
                    avg_nodes[protocol][index].append(int(count))
                    index += 1

    avg_nodes_per_time = {protocol: [] for protocol in protocols}
    for protocol in protocols:
        for time in sorted(avg_nodes[protocol].keys()):
            avg_nodes_per_time[protocol].append(
                sum(avg_nodes[protocol][time]) / len(avg_nodes[protocol][time])
            )

    plt.figure(figsize=(10, 6))
    for i, protocol in enumerate(protocols):
        plt.plot(
            range(len(avg_nodes_per_time[protocol])),
            avg_nodes_per_time[protocol],
            label=protocol,
            color=colors[i],
        )

    plt.title("Network lifetime", weight="bold")
    plt.xlabel("Time (s)")
    plt.ylabel("Number of Nodes (#)")
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.legend()
    plt.grid(True)
    plt.savefig(f"output-remaining_nodes.pdf")
    plt.close()


def save_aodvv2_uml():
    if not os.path.exists("src/aodvv2/uml"):
        os.makedirs("src/aodvv2/uml")

    for file in os.listdir("src/aodvv2/model"):
        if file.endswith(".h"):
            os.system(
                f"hpp2plantuml -i src/aodvv2/model/{file} -o src/aodvv2/uml/{file.replace('.h','.puml')}"
            )


def save_stats(protocol):
    route_map = {}
    with open(input_route_path, "r") as file:
        lines = file.readlines()
        for line in lines[1:]:
            src_ip, dst_ip, hops = line.strip().split(",")
            route_map[(src_ip, dst_ip)] = hops

    with open(input_ping_path, "r") as file:
        lines = file.readlines()
        ping_map = {}
        for line in lines[1:]:
            src_ip, dst_ip, value = line.strip().split(",")
            key = (src_ip, dst_ip)
            if key not in ping_map:
                ping_map[key] = []
            ping_map[key].append(float(value))

    output_file_path = output_ping_path.replace("{protocol}", protocol)
    with open(output_file_path, "a") as file:
        for key, values in ping_map.items():
            avg_ping = round(sum(values) / len(values), 3)
            hops = route_map[key] if key in route_map else 0
            file.write(
                f"{key[0]},{key[1]},{hops},{values[0]},{min(values)},{max(values)},{avg_ping}\n"
            )
        file.write(f"-,-,-,-,-,-,-\n")

    with open(input_packets_path, "r") as file:
        lines = file.readlines()
        packets = {}
        for line in lines[1:]:
            type, size = line.strip().split(",")
            if type not in packets:
                packets[type] = []
            packets[type].append(int(size))

    output_file_path = output_packets_path.replace("{protocol}", protocol)
    with open(output_file_path, "a") as file:
        for p in packet_types:
            if p not in packets:
                packets[p] = [0]
            avg_size = round(sum(packets[p]) / len(packets[p]), 3)
            file.write(f"{p},{len(packets[p])},{avg_size},{min(packets[p])},{max(packets[p])}\n")


def clean_folder():
    for file in os.listdir(folder_path):
        file_path = os.path.join(folder_path, file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
        except Exception as e:
            print(e)
    print("\nFolder cleaned!")


if __name__ == "__main__":
    menu()
