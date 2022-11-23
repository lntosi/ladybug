FROM lntosi/ladybug-sw:latest
LABEL Description="Docker image for Developers"
ENV DEBIAN_FRONTEND noninteractive

ARG username
ARG uid
ARG gid

RUN groupadd -g $gid $username
RUN useradd -ms /bin/bash --gid $gid --uid $uid $username && adduser $username sudo && echo -n "$username:psswrd" | chpasswd

USER $username
WORKDIR /data/datasets-raw/
CMD "/bin/bash"
