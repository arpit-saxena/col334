import argparse
import glob
import os
import pandas as pd
from matplotlib import pyplot as plt


def main():
    parser = argparse.ArgumentParser(description="Plot traces of congestion window")
    parser.add_argument(
        "traceDir", type=str, help="Directory in which the traces are stored"
    )

    args = parser.parse_args()
    os.chdir(args.traceDir)

    for cwndFile in glob.glob("*.cwnd"):
        protocolName = os.path.splitext(cwndFile)[0]
        dropsFile = protocolName + ".drops"

        drops = pd.read_csv(dropsFile)
        numDrops = drops.shape[0]

        cwndDf = pd.read_csv(
            cwndFile, sep="\t", names=["time", "Congestion Window Size"]
        )
        fig, ax = plt.subplots()
        cwndDf.plot.line(ax=ax, x=0, y=1, ylabel="Congestion window size", marker=".")
        ax.set_title(
            f"Congestion Window Size for TCP {protocolName}\nNumber of drops: {numDrops}"
        )
        ax.set_xlabel(f"Time (seconds)")

        fig.savefig(protocolName + ".png")


if __name__ == "__main__":
    main()
