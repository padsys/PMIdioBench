# Persistent Lock-Free Ring Buffer

This folder contains the source for two lock-free persistent ring buffer designs – a
transaction-based design, TX, for ADR-based PMEM systems and a transaction-free design,
TX-free, for eADR-based PMEM systems. Our designs are based on
[Krizhanovsky’s algorithm](https://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer)
for volatile lock-free ring buffers. To the best of our knowledge, these are the first
lock-free persistent ring buffer solutions available in the community. 

# In this readme:

* [Prerequisites](#prerequisites)
* [Installation](#install)
* [Running Tests](#tests)

<a id="prerequisites"></a>
## Prerequisites

* [PMDK](https://github.com/pmem/pmdk)
* [libpmemobj-cpp](https://github.com/pmem/libpmemobj-cpp)
* PMIdioBench

<a id="install"></a>
## Installation

* Set configuration parameters in include/config.h and scripts/run_all.sh
* ```make```

<a id="tests"></a>
## Running Tests

```scripts/run_all.sh```
