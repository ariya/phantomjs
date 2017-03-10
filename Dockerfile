FROM ubuntu:14.04
MAINTAINER Matthieu Paret <matthieu@ifeelgoods.com>
# sudo docker run -v /home/matt/dev/phantomjs:/home/app 8c8a6dc2fd33 ./build.sh --confirm 

RUN apt-get update
RUN apt-get install -y build-essential g++ flex bison gperf ruby perl libsqlite3-dev libfontconfig1-dev libicu-dev libfreetype6 libssl-dev libc-dev libpng-dev libjpeg-dev python libx11-dev libxext-dev

RUN mkdir -p /home/app
ENV HOME /home/app
WORKDIR /home/app

CMD ["./build.sh"]
