FROM ubuntu:latest

ADD ./MPSU /app/MPSU

ADD ./APSI /app/APSI

ADD ./vcpkg /app/vcpkg

WORKDIR /app

RUN apt-get update && apt-get -y install python3 cmake build-essential

RUN apt-get -y install curl zip unzip tar git pkg-config

# RUN cd /app/vcpkg/ && ./bootstrap-vcpkg.sh 

RUN cd /app/vcpkg/ && ./vcpkg install seal[no-throw-tran] kuku log4cplus cppzmq flatbuffers jsoncpp gtest tclap

RUN mkdir /app/APSI/build && cd /app/APSI/build && cmake .. -DCMAKE_TOOLCHAIN_FILE=/app/vcpkg/scripts/buildsystems/vcpkg.cmake && make -j4 

RUN mkdir /app/MPSU/build && cd /app/MPSU/build && cmake .. && make -j4 

ENTRYPOINT ["python3", "/app/exec.py"]
