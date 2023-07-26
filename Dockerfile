FROM amazoncorretto:17-alpine3.17-jdk

RUN mkdir -p "/tmp/agent"
RUN cd /tmp/agent
ADD CMakeLists.txt /tmp/agent/CMakeLists.txt
ADD src /tmp/agent/src
WORKDIR /tmp/agent/
RUN apk add cmake alpine-sdk
RUN mkdir release && cd release
