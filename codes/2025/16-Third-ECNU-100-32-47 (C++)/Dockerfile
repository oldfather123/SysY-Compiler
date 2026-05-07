FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# Basic build tools
RUN sed -i 's@http://archive.ubuntu.com/@http://mirrors.tuna.tsinghua.edu.cn/@g' /etc/apt/sources.list && \
    sed -i 's@http://security.ubuntu.com/@http://mirrors.tuna.tsinghua.edu.cn/@g' /etc/apt/sources.list && \
    apt clean && apt update
RUN apt install -y build-essential clang-12 git sudo wget vim cmake tar clangd-12 ninja-build qemu-system-misc flexc++ bisonc++

RUN useradd -ms /bin/bash -G sudo aaac &&\ 
    passwd -d aaac
USER aaac
WORKDIR /home/aaac
CMD ["/bin/bash"]
