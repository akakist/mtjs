FROM   ubuntu:latest AS builder


USER root

RUN apt-get update && \
    apt-get install -y cmake  g++ libssl-dev libsqlite3-dev git pkg-config libpq-dev libmariadb-dev nlohmann-json3-dev librocksdb-dev/noble python3 clang libgmp-dev libsodium-dev libsqlitecpp-dev libsqlite3-dev \
    libzstd-dev liblz4-dev libbz2-dev libsnappy-dev\
 &&  apt-get clean \
 &&  rm -rf /var/lib/apt/lists/*



#COPY usr/local/include/ /usr/local/include/
#COPY usr/local/lib /usr/local/lib
#RUN git clone https://github.com/skalenetwork/libBLS.git && cd libBLS && mkdir build && cd build && cmake .. && make -j4 install
RUN git clone --recursive https://github.com/supranational/blst.git
RUN cd blst && ./build.sh && cp  bindings/*.h /usr/local/include && cp libblst.a /usr/local/lib 

COPY ./src /root/src



RUN cd /root/src && mkdir build1 && cd build1 && cmake .. && make install -j 6  && ldconfig  && rm -r /root/src /opt && strip /usr/local/bin/mtjs

FROM alpine:3.19

COPY --from=builder /usr/local/bin/mtjs  /usr/local/bin/mtjs
COPY ./src/js/ts /mtjs
RUN apk add --no-cache apache2-utils busybox-extras curl
#ENV PATH=/usr/local/bin:$PATH