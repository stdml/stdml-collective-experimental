#!/usr/bin/env -S sh -c 'docker build --rm -t stdml-collective-experimental:snapshot .'
FROM ubuntu:focal

RUN apt update
RUN apt install -y make build-essential

ADD test.cpp /
RUN g++ -std=c++17 -c test.cpp
