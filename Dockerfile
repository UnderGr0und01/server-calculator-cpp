FROM centos:7

RUN yum update -y && \
    yum install -y epel-release && \
    yum install -y libpqxx-devel && \
    yum install -y gcc-c++ cmake make

COPY . /server-calculator-cpp/
RUN mkdir build
WORKDIR /server-calculator-cpp/build

RUN cmake .. &&\
    cmake --build .

RUN chmod +x server-calculator-cpp

CMD ["./server-calculator-cpp"]