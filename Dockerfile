FROM ubuntu:20.10

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=en_US.UTF-8 \
    LC_ALL=C.UTF-8 \
    LANGUAGE=en_US.UTF-8

RUN apt-get update && apt-get install -y bwa build-essential libz-dev libboost-all-dev

COPY . /opt/bin/rnapattmatch/
WORKDIR /opt/bin/rnapattmatch/
RUN make

CMD ["bin/rnapattmatch", "-h"]
