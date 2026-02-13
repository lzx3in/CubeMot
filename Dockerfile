FROM debian:trixie

# 设置环境变量
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# 更新软件源并安装必要的构建工具
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y \
    # 基础构建工具
    git \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    python3-venv \
    make \
    # ARM嵌入式工具链
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    libnewlib-arm-none-eabi \
    # 调试工具
    gdb-multiarch \
    openocd \
    # 其他实用工具
    wget \
    ca-certificates \
    xz-utils \
    bzip2 \
    file \
    tree \
    python3-kconfiglib \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# 创建非root用户用于构建
RUN useradd -m -s /bin/bash builder && \
    usermod -aG sudo builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# 切换到builder用户
USER builder
WORKDIR /home/builder

# 克隆CubeMot项目
RUN git clone https://github.com/lzx3in/CubeMot.git

WORKDIR /home/builder/CubeMot

# 准备构建配置
RUN alldefconfig Kconfig

# 执行GCC Debug构建
RUN cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/gcc_arm_none_eabi_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBOARD=nucleo_g431rb \
    -B build/Debug && \
    cmake --build build/Debug

# 验证构建输出
RUN echo "=== 构建输出验证 ===" && \
    ls -lh target/nucleo_g431rb/Debug/ && \
    file target/nucleo_g431rb/Debug/CubeMot.elf && \
    arm-none-eabi-size target/nucleo_g431rb/Debug/CubeMot.elf

WORKDIR /home/builder/CubeMot

# 默认命令
CMD ["/bin/bash"]
