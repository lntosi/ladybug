FROM ubuntu:16.04
LABEL Description="LadyBug"
ENV DEBIAN_FRONTEND noninteractive

#Update the SO
RUN apt update && apt upgrade -y

COPY ./*.deb /tmp

#General Requirements
RUN apt-get update && \
  apt-get install -y \
  software-properties-common \
  sudo \
  dpkg \
  libicu55 \
  xsdcxx \
  freeglut3 \
  freeglut3-dev \
  libswscale-dev \
  libavcodec-dev \
  libavformat-dev  &&\
  apt-get clean && \
  rm -rf /var/lib/apt/lists/*

RUN dpkg -i /tmp/libxerces-c3.1_3.1.3+debian-1_amd64.deb

#LadyBug
RUN dpkg -i /tmp/dpkg -i ladybug-1.16.3.48_amd64.deb
