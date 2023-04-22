set -euo pipefail
freq="150"
sleep_time="10"

perf record -F $freq -g -p $(pidof srv) --proc-map-timeout=10000 -- sleep $sleep_time && perf script > /tmp/out.perf

stackcollapse-perf.pl /tmp/out.perf | c++filt > /tmp/out.folded

svg="./flames/$(date +%m-%d_%H%M%S).svg"
flamegraph.pl /tmp/out.folded > $svg
