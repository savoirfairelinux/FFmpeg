FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

RUN     apt-get update -qq \
        && apt-get -y install \
        autoconf \
        automake \
        build-essential \
        cmake \
        git-core \
        libass-dev \
        libfreetype6-dev \
        libgnutls28-dev \
        libsdl2-dev \
        libtool \
        libva-dev \
        libvdpau-dev \
        libvorbis-dev \
        libxcb1-dev \
        libxcb-shm0-dev \
        libxcb-xfixes0-dev \
        pkg-config \
        texinfo \
        wget \
        yasm \
        zlib1g-dev \
        nasm \
        libx264-dev

ENV PREFIX=/usr/local
ENV PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
