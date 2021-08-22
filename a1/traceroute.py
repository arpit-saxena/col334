import argparse
import matplotlib.pyplot as plt
from scapy.all import sr, IP, ICMP


def get_res_and_rtt(dest: str, ttl: int):
    res = sr(IP(dst=dest, ttl=ttl)/ICMP(), verbose=False, timeout=4)[0]
    if len(res) == 0: # Timed out
        return None, 0
    query, answer = res[0] # Get the answered QueryResult
    return answer, answer.time - query.sent_time


def run_traceroute(dest: str, max_ttl: int):
    ttl = 1

    # See https://datatracker.ietf.org/doc/html/rfc792 for details of
    # response type and code
    while ttl <= max_ttl:
        res, rtt = get_res_and_rtt(dest, ttl)
        if res is None: # Timed out
            yield 0
        elif res[ICMP].type == 11 and res[ICMP].code == 0:
            # type 11 <=> Time exceeded message
            # Then code 0 <=> time to live exceeded in transit
            yield rtt, res.src
        elif res[ICMP].type == 0:
            # type 0 <=> echo reply message
            yield rtt, res.src
            break
        else:
            # Some other (error) response, we'll consider same as timeout
            yield 0
        ttl += 1


def print_and_plot_trace(dest: str, max_ttl: int, outFile: str):
    hop = 1
    x = []
    y = []

    print(f"traceroute to {dest}, {max_ttl} hops max")
    for record in run_traceroute(dest, max_ttl):
        x.append(hop)
        if record == 0: # Timed out or equivalent
            print(f"{hop}\t*")
            y.append(0)
        else:
            rtt, src = record
            print(f"{hop}\t{src}\t{rtt*1000:.3f} ms")
            y.append(rtt * 1000)
        hop += 1

    plt.plot(x, y, "bd-", mfc="red", mec="black")
    plt.xlabel("Hop Number")
    plt.ylabel("Round Trip Time (ms)")
    plt.title(f"traceroute to {dest}")
    plt.savefig(outFile)


def main():
    parser = argparse.ArgumentParser(description="print the route packets trace to network host and plot the RTT for each host in between")
    parser.add_argument("dest", metavar="example.com", type=str, help="the destination to which packets are traced")
    parser.add_argument("-o", "--out", default="plot.png", help="filename to which the rtt vs hop plot will be saved")
    parser.add_argument("-m", '--max-ttl', type=int, default=30, help="max TTL value that will be used for outgoing ICMP packets")

    args = parser.parse_args()
    print_and_plot_trace(args.dest, args.max_ttl, args.out)


if __name__ == "__main__":
    main()
