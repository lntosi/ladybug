FROM ubuntu:16.04
LABEL Description="LadyBug"
ENV DEBIAN_FRONTEND noninteractive

COPY ./*.deb /tmp

#General Requirements
RUN apt-get update && apt-get install -yq \
  software-properties-common \
  sudo \
  wget \
  gcc \
  g++ \
  libraw1394-tools \
  libraw1394-dev \
  libc-dev-bin \
  libusb-1.0-0-dev \
  make \
  dpkg \
  libicu55 \
  xsdcxx \
  freeglut3 \
  freeglut3-dev \
  libswscale-dev \
  libavcodec-dev \
  libavformat-dev \
  tzdata \
  && ln -fs /usr/share/zoneinfo/Europe/Brussels /etc/localtime \
  && dpkg-reconfigure -f noninteractive tzdata \
  && rm -rf /var/lib/apt/lists/*

RUN dpkg -i /tmp/libxerces-c3.1_3.1.3+debian-1_amd64.deb

#LadyBug
RUN dpkg -i /tmp/ladybug-1.16.3.48_amd64.deb; exit 0
