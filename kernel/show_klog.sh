#!/bin/bash
#
# zkx@20201107
#
dmesg | tail -n 1000 | grep $1
