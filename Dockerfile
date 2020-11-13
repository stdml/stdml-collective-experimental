#!/usr/bin/env -S sh -c 'docker build --rm -t stdml-collective-experimental:snapshot .'
FROM ubuntu:focal

RUN apt update
RUN apt install -y make cmake g++-10

RUN apt install -y golang-go
RUN apt install -y git
RUN go get -v github.com/lsds/KungFu/srcs/go/cmd/kungfu-run
RUN go install -v github.com/lsds/KungFu/srcs/go/cmd/kungfu-run

ENV PATH /root/go/bin:${PATH}

WORKDIR /src
ADD . .

RUN ./configure
RUN make
RUN ./scripts/run/example-1.sh
