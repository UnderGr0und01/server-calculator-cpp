#FROM centos:7
#
#RUN yum update -y && \
#    yum install -y epel-release && \
#    yum install -y boost-devel && \
#    yum install -y postgresql-libs && \
#    yum install -y libpqxx-devel && \
#    yum install -y gcc-c++ cmake make
#
#COPY . /server-calculator-cpp/
#RUN mkdir build
#WORKDIR /server-calculator-cpp/build
#
#RUN cmake .. &&\
#    cmake --build .
#
#RUN chmod +x server-calculator-cpp
#
#CMD ["./server-calculator-cpp"]

# Use a base image with CMake and GCC
FROM ubuntu:20.04


RUN apt-get update -y && \
    apt-get -y build-essential && \
    apt-get -y cmake && \
    atp-get -y libboost-all-dev && \
    apt-get -y libpq-dev


WORKDIR /app


COPY . .


RUN mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build .


CMD ["./build/server-calculator-cpp"]