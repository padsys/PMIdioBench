# PMIdioBench

Identifying generic PMEM characteristics is non-trivial. This is because existing
benchmarks only provide the ability to evaluate performance characteristics but
not isolate their root cause. To fully understand the nuanced behavior of PMEM,
we designed a microbenchmark suite called PMIdioBench. The suite consists of a set
of targeted experiments to evaluate PMEM performance and identify its
idiosyncrasies. PMIdioBench pins threads to cores, disables cache prefetching (when
required), and primes the CPU cache to produce accurate performance measurements.
It also includes two tools to help identify the main reason behind performance
anomalies and bottlenecks – one to examine relevant hardware counters and display
useful metrics (such as EWR, EBR, and RWR) and the other to generate benchmark
trace and distill gathered information in the form of a flame graph. PMIdioBench can
be used to calculate the quantitative impact of the idiosyncrasies on both current
and future PMEM.

# In this readme:

* [Prerequisites](#prerequisites)
* [Installation](#install)
* [Running Idiosyncrasy Tests](#itests)
* [Running NUMA Study](#numa-study)

<a id="prerequisites"></a>
## Prerequisites

* [PMDK](https://github.com/pmem/pmdk)
* [libpmemobj-cpp](https://github.com/pmem/libpmemobj-cpp)
* MongoDB with [PMSE](https://github.com/pmem/pmse) commit d8372274a9b3f18dbceb0cfcc0caa496b5940789
* Java
* [fio](https://github.com/axboe/fio)
* [YCSB](https://github.com/brianfrankcooper/YCSB)
* [Intel VTune Profiler](https://software.intel.com/content/www/us/en/develop/tools/vtune-profiler/choose-download.html#standalone)
* [FlameGraph](https://github.com/brendangregg/FlameGraph)
* [ipmctl](https://github.com/intel/ipmctl)

<a id="install"></a>
## Installation

* Apply pmse.patch to PMSE source before compiling it
* ```cp scripts/config.sh.in scripts/config.sh```
* Set configuration parameters in config.sh
* ```make```

<a id="itests"></a>
## Running Idiosyncrasy Tests

```scripts/run_all.sh```

<a id="numa-study"></a>
## Running NUMA Study

```scripts/numa_study.sh```
