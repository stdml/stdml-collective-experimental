# docker build --rm -t stdml-collective-experimental:snapshot .
FROM ubuntu:focal

RUN apt update
RUN apt install -y make cmake g++-10

WORKDIR /src
ADD . .

RUN ./configure
RUN make
