#!/bin/bash

yum install -y yum-utils rpm-build
cd /src/deploy
./mkrpm.sh
