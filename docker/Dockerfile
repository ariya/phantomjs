FROM ubuntu:16.04

ENV VERSION 2.1.1

RUN apt-get update && apt-get upgrade -y &&\
    apt-get install -y libfreetype6 libfontconfig

ADD phantomjs /usr/bin/phantomjs
CMD ["/usr/bin/phantomjs"]

