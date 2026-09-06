FROM ubuntu:24.04

USER root

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        ninja-build \
        pkg-config \
        gdb \
        git \
        git-lfs \
        clang \
        valgrind \
        gstreamer1.0-libav \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-tools \
        gstreamer1.0-plugins-base-apps \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer1.0-dev \
        libncurses-dev \
        iputils-ping \
        iproute2 \
        dnsutils \
        tcpdump \
        tshark \
        ethtool \
        lsof \
        strace \
        socat \
        netcat-openbsd \
        ffmpeg \
        curl \
        mtr \
        net-tools \
        tree \
        jq \
        less \
        file \
        unzip \
        zip \
        openssh-server \
        sudo \
    && rm -rf /var/lib/apt/lists/* \
    && echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

USER ubuntu 
