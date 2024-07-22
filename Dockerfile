FROM debian:bookworm-slim AS builder

RUN	apt-get update && apt-get install -y build-essential git libncurses5-dev libssl3 libc6 autoconf
RUN cd /opt && git clone https://github.com/edwardchuang/qkmj.git
RUN cd /opt/qkmj  && ./configure && make 

FROM edwardchuang/wsproxy:latest

ENV	BUILD_DEPS="inetutils-inetd inetutils-telnetd" \
	QKMJ_SERVER="0.0.0.0 7001" \
	MJQPS_DAEMON_PORT=7001 \
	WSPROXY_ADDR="0.0.0.0:23" \
	TERMINFO="/lib/terminfo" \
	TERM=vt102

RUN	apt-get update && apt-get install -y ${BUILD_DEPS}
RUN mkdir -p /opt/qkmj
COPY --from=builder /opt/qkmj/mjgps /opt/qkmj/mjgps
COPY --from=builder /opt/qkmj/qkmj /opt/qkmj/qkmj 
RUN mkdir -p /tmp/qkmj && chown -R games:games /tmp/qkmj
	
EXPOSE	80
VOLUME	[ "/tmp/qkmj" ]
CMD	echo "telnet\tstream\ttcp\tnowait\tgames:games\t/usr/sbin/tcpd\t/usr/sbin/telnetd -E '/opt/qkmj/qkmj ${QKMJ_SERVER}'"> /etc/inetd.conf && inetutils-inetd && nginx -g 'daemon on;' && /opt/qkmj/mjgps ${MJQPS_DAEMON_PORT}
