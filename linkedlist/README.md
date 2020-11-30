# Persistent Lock-Free Linkedlist

This folder contains the source for two lock-free persistent linkedlist designs –
one design for ADR-based PMEM systems, called TLog, and another for eADR-based PMEM
systems, called Log-free. Our lock-free linkedlist designs are extensions of Harris’
algorithm and support the same basic operations – insert, delete, and contains.

# In this readme:

* [Prerequisites](#prerequisites)
* [Installation](#install)
* [Running Tests](#tests)

<a id="prerequisites"></a>
## Prerequisites

* [PMDK](https://github.com/pmem/pmdk)
* PMIdioBench

<a id="install"></a>
## Installation

* Set configuration parameters in include/config.h and scripts/run_all.sh
* ```make```

<a id="tests"></a>
## Running Tests

```scripts/run_all.sh```

The script will run all experiments and print throughput results. Latency results will be
written to adr-latency.log and eadr-latency.log. Flame graphs for all runs will also be
produced as svg files.
