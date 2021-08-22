# COL334 Assignment 1

Submitted by Arpit Saxena, 2018MT10742

## Running traceroute

It is recommended to create a Python virtual environment. After activating it, run `pip install -r requirement.txt` to install the dependencies.

To get information about various arguments, run `python traceroute.py --help`

To use default arguments and trace a route to google.com, run `python traceroute.py google.com`. This will output the route trace and the RTT vs hop plot will be saved as `plot.png`

NOTE: The script needs root privileges since it uses Scapy which creates raw sockets. If not, the `CAP_NET_RAW` capability needs to be set. See [this](https://stackoverflow.com/a/22426121/5585431) for details.
