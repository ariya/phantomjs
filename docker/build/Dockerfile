FROM ubuntu:16.04

ENV VERSION 2.1.1
ENV OPEN_SSL_VERSION 1.0.2h

RUN apt-get update && apt-get install -y libfreetype6 libfontconfig build-essential wget python git libicu-dev bison ruby flex gperf
RUN cd /tmp && wget https://www.openssl.org/source/openssl-$OPEN_SSL_VERSION.tar.gz
RUN cd /tmp && tar -xzvf openssl-$OPEN_SSL_VERSION.tar.gz
RUN cd /tmp/openssl-$OPEN_SSL_VERSION && ./config --prefix=/usr && make depend && make install
RUN cd /tmp && git clone https://github.com/ariya/phantomjs.git
RUN cp -Lr /tmp/openssl-$OPEN_SSL_VERSION/include/openssl /usr/include/

RUN cd /tmp/phantomjs && git checkout $VERSION

RUN cd /tmp/phantomjs/ && ./build.py -r -c

RUN cp /tmp/phantomjs/bin/phantomjs /bin/phantomjs


