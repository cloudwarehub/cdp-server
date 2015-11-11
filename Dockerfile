FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y xorg xserver-xorg-video-dummy wget
RUN apt-get install -y x264 libxcb1 libxcb-composite0
RUN wget https://gist.githubusercontent.com/guodong/91b631bdfa42e5e72f21/raw/934da2c2d506d1426cac15bf066144407d3a7161/xorg-dummy.conf -O /root/xorg.conf
ADD ./start.sh /root/start.sh
ADD ./dest/cdp-server /root/cdp-server
RUN chmod u+x /root/start.sh
EXPOSE 5999/udp 6000
ENV DISPLAY :0
CMD /root/start.sh