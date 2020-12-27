FROM alpine:3.12.3

RUN apk update && \
    apk add --no-cache autoconf build-base binutils cmake curl file gcc g++ git libgcc libtool linux-headers make musl-dev ninja tar unzip wget boost-dev

COPY . /opt/rnapattmatch/
WORKDIR /opt/rnapattmatch/
RUN make

FROM alpine:3.12.3

RUN apk update && apk add --no-cache boost
RUN mkdir /opt/bin ; mkdir /data
COPY --from=0 /opt/rnapattmatch/bin/RNAPattMatch /usr/bin/
COPY --from=0 /opt/rnapattmatch/bin/RNAdsBuilder /usr/bin/

WORKDIR /data

CMD ["RNAPattMatch", "-h"]
