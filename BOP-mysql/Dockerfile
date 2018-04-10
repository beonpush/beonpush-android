FROM debian:jessie

LABEL version="2.0.0"
LABEL description="Beonpush-MySQL Container"

# Install dependencies
RUN apt-get update && \
    apt-get dist-upgrade -y && \
    DEBIAN_FRONTEND="noninteractive" apt-get -f -q upgrade -y -o Dpkg::Options::="--force-confnew" --no-install-recommends && \
    DEBIAN_FRONTEND="noninteractive" apt-get -f -q install -y -o Dpkg::Options::="--force-confnew" \
    build-essential libpgm-5.1 libdb++-dev libevent-pthreads-2.0-5 libevent-dev \
    libboost-all-dev libssl-dev libqt4-dev libminiupnpc-dev \
    libqrencode-dev libtool bsdmainutils \
    libleveldb-dev libmysqlcppconn-dev

# Install Code
RUN cd /var && mkdir /var/beonpush-src && mkdir /var/beonpush-data
COPY . /var/beonpush-src
RUN chmod +x /var/beonpush-src/docker-entrypoint.sh
RUN cd /var/beonpush-src/src && make clean -f makefile.unix && make -f makefile.unix; exit 0

# Environment variables -- CHANGE ME !!
ENV RPCUSER ""
ENV RPCPASSWORD ""
ENV DBHOST ""
ENV DBUSER ""
ENV DBPASS ""
ENV DBNAME ""

# RPC Port
EXPOSE 34121

# P2P Port
EXPOSE 32988

# Data dir
VOLUME ["/var/beonpush-data"]

ENTRYPOINT ["/var/beonpush-src/docker-entrypoint.sh"]
