FROM ubuntu:18.04

LABEL author="Safronov 221-329"

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update
RUN apt-get install qt5-default qtbase5-dev qt5-qmake -y
RUN apt-get install build-essential -y

WORKDIR /221-329_Safronov_Evgenii
COPY *.cpp .
COPY *.h .
COPY *.pro .

RUN qmake gameServer.pro
RUN make

ENTRYPOINT ["./gameServer"]