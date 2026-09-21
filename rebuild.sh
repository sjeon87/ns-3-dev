#!/usr/bin/env bash
./ns3 clean && ./ns3 configure --enable-tests --enable-examples > configure.log 2>&1 && ./ns3 build > build.log 2>&1
