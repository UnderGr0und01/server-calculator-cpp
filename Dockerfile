FROM centos:7

RUN yum update -y && \
    yum install -y lksctp-tools-devel && \
    yum install -y epel-release && \
    yum install -y gcc-c++ cmake make libpqxx-devel

COPY . /app
WORKDIR app/build

RUN cmake .. &&\
    make

ENTRYPOINT ["./server-calculator-cpp"]