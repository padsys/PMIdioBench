#!/bin/bash

source scripts/config.sh

PMEM_PATH=$PMEM_DIR1

# Disable hw prefetching
disable_prefetch/intel-prefetch-disable > /dev/null 2>&1
sleep 2

# Run test_i1
rm -f $PMEM_PATH/test /dev/shm/test
sleep 2
numactl -N 0 ./test_i1.x /dev/shm/test 0
sleep 2
numactl -N 0 ./test_i1.x /dev/shm/test 1
sleep 2
numactl -N 0 ./test_i1.x $PMEM_PATH/test 0
sleep 2
numactl -N 0 ./test_i1.x $PMEM_PATH/test 1
sleep 2

# Enable hw prefetching
disable_prefetch/intel-prefetch-disable -e > /dev/null 2>&1
sleep 2

# Run test_i2
scripts/test_i2.sh
sleep 2

# Run test_i3
scripts/test_i3.sh
sleep 2

# Run test_i4
scripts/test_i4.sh
sleep 2

# Run test_i5
scripts/test_i5.sh
sleep 2

# Run test_i6
scripts/test_i6.sh
sleep 2

# Run test_i7
rm -f $PMEM_PATH/test /dev/shm/test
sleep 2
scripts/ipmw.sh numactl -N 0 ./test_i7.x $PMEM_PATH/test
sleep 2

# Disable hw prefetching
disable_prefetch/intel-prefetch-disable > /dev/null 2>&1
sleep 2

# Run test_i8
rm -f $PMEM_PATH/test /dev/shm/test
sleep 2
numactl -N 0 ./test_i8.x /dev/shm/test 0
sleep 2
numactl -N 0 ./test_i8.x /dev/shm/test 1
sleep 2
numactl -N 0 ./test_i8.x $PMEM_PATH/test 0
sleep 2
numactl -N 0 ./test_i8.x $PMEM_PATH/test 1
sleep 2

# Enable hw prefetching
disable_prefetch/intel-prefetch-disable -e > /dev/null 2>&1
sleep 2

# Cleanup
rm -f $PMEM_PATH/test /dev/shm/test
