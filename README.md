# Artifact Overview

This repository contains the code for our VLDB'21 paper,
"Shashank Gugnani, Arjun Kashyap, and Xiaoyi Lu. Understanding the Idiosyncrasies of Real Persistent Memory".

We provide the following in this repository:

* Source for PMIdioBench, the micro-benchmark suite proposed in Section 3
* Source for ring buffer designs proposed in Section 6
* Source for linkedlist designs proposed in Section 6
* A set of scripts to reproduce idiosyncrasy, numa study, and lock-free results presented in Sections 4, 5, and 6, respectively
* A set of scripts and data files to plot all graphs in the paper

# Directory Layout

* Bench/ contains the source for idiosyncrasy benchmarks
* ring-buffer/ contains the source for TX and TX-free ring buffer designs
* linkedlist/ contains the source for TLog and Log-free linkedlist designs
* Data/ contains Excel files containing data points for all experimental analysis in the paper
* Graphs/ contains PDF files of all graphs in the paper
* Scripts/ contains scripts to plot all graphs in the paper

# System Requirements

* Linux x86_64 server
* PMEM formatted as DAX FS
* At least 2 NUMA nodes (required for I4 and NUMA study experiments)

# Steps to reproduce idiosyncrasy and NUMA study results

* Follow instructions in Bench/README.md
* Copy idiosyncrasy results to Data/pmem-idiosyncrasies.xlsx
* Copy NUMA study results to Data/numa-study.xlsx

# Steps to reproduce ring buffer results

* Follow instructions in ring-buffer/README.md
* Copy results to Data/lock-free-results.xlsx

# Steps to reproduce TLog and Log-free linkedlist results

* Follow instructions in linkedlist/README.md
* Copy results to Data/lock-free-results.xlsx

# Steps to reproduce SOFT and Link-free linkedlist results

* ```git clone https://github.com/shashankgugnani/Efficient-Lock-Free-Durable-Sets.git```
* ```cd Efficient-Lock-Free-Durable-Sets```
* ```Scripts/testLists.sh```
* The script will run all experiments and print throughput results. Latency results will be
  written to adr-latency.log and eadr-latency.log.
* Copy results to Data/lock-free-results.xlsx

# Steps to plot all graphs in the paper

* Prerequisites: Python installation with jupyter notebook, matplotlib, and xlrd packages
* Open Scripts/graphs.ipynb in jupyter notebook
* Run the complete script
* Plotted graphs will be available in the Graphs directory

# Reference

Please cite PMIdioBench in your publications if it helps your research:

```
@article{gugnani-vldb21,
  author = {Gugnani Shashank and Kashyap Arjun and Lu Xiaoyi},
  title = {Understanding the Idiosyncrasies of Real Persistent Memory},
  journal = {Proceedings of the VLDB Endowment},
  volume = {14},
  number = {4},
  year = {2021},
}
```
