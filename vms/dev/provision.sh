#!/bin/sh
set -e

export DEBIAN_FRONTEND=noninteractive

sudo apt update
sudo apt install -y git
sudo apt install -y tree

sudo apt install -y make cmake g++-10
