FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    wget \
    gnupg \
    software-properties-common \
    lsb-release \
    build-essential \
    unzip \
    curl \
    gcc-riscv64-linux-gnu \
    qemu-user

RUN wget https://apt.llvm.org/llvm-snapshot.gpg.key -O - | gpg --dearmor -o /usr/share/keyrings/llvm.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/noble/ llvm-toolchain-noble-18 main" \
    > /etc/apt/sources.list.d/llvm.list && \
    apt-get update && \
    apt-get install -y clang-18 lld-18 llvm-18 llvm-18-dev clangd-18

RUN pip install pre-commit --break-system-packages

ENV PATH="/usr/lib/llvm-18/bin:${PATH}"
ENV CC=clang-18
ENV CXX=clang++-18
ENV LLVM_CONFIG=/usr/lib/llvm-18/bin/llvm-config

RUN ln -sf /usr/lib/llvm-18/bin/clangd /usr/bin/clangd && \
    ln -sf /usr/lib/llvm-18/bin/clang /usr/bin/clang && \
    ln -sf /usr/lib/llvm-18/bin/clang++ /usr/bin/clang++ && \
    ln -sf /usr/lib/llvm-18/bin/lli /usr/bin/lli && \
    ln -sf /usr/lib/llvm-18/bin/llvm-config /usr/bin/llvm-config && \
    ln -sf /usr/lib/llvm-18/bin/opt /usr/bin/opt && \
    ln -sf /usr/lib/llvm-18/bin/llc /usr/bin/llc


WORKDIR /workspace
RUN chown -R vscode:vscode /workspace
USER vscode
