FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        gdb \
        git \
        gstreamer1.0-libav \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-tools \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer1.0-dev \
        libncurses-dev \
        neovim \
        ninja-build \
        openssh-server \
        pkg-config \
        sudo \
        valgrind \
    && rm -rf /var/lib/apt/lists/*


