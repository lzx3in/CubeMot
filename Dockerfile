FROM debian:trixie

WORKDIR /workspace

COPY . /workspace/

RUN bash tools/setup_cubemot_env.sh

RUN cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/gcc_arm_none_eabi_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBOARD=nucleo_g431rb \
    -B build/Debug && \
    cmake --build build/Debug

CMD ["/bin/bash"]
