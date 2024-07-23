FROM debian:bookworm-slim AS builder

RUN	apt-get update && apt-get install -y build-essential git libncurses-dev libncurses6 libncursesw6 libssl3 libc6 autoconf
RUN cd /opt && git clone --depth 1 https://github.com/edwardchuang/qkmj.git
RUN cd /opt/qkmj  && ./configure && make 

FROM edwardchuang/wsproxy:latest

ENV	BUILD_DEPS="inetutils-inetd inetutils-telnetd libncurses-dev libncurses6 libncursesw6 locales" \
	QKMJ_SERVER="0.0.0.0 7001" \
	MJQPS_DAEMON_PORT=7001 \
	WSPROXY_ADDR="0.0.0.0" \
	WSPROXY_PORT="7001" \
	TERMINFO_DIRS="/lib/terminfo" \
	TERMINFO="/lib/terminfo" \
	TERM=xterm-256color \
	LANG=en_US.UTF-8 \
	LC_ALL=en_US.UTF-8

RUN	apt-get update && apt-get install -y ${BUILD_DEPS} && apt-get autoremove
RUN mkdir -p /opt/qkmj
COPY --from=builder /opt/qkmj/mjgps /opt/qkmj/mjgps
COPY --from=builder /opt/qkmj/qkmj /opt/qkmj/qkmj 
RUN mkdir -p /tmp/qkmj && chown -R games:games /tmp/qkmj
RUN chmod 755 /opt/qkmj/mjgps
RUN chmod 755 /opt/qkmj/qkmj
RUN echo "LC_ALL=en_US.UTF-8" >> /etc/environment
RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "LANG=en_US.UTF-8" > /etc/locale.conf
RUN locale-gen en_US.UTF-8

EXPOSE	23 80 8080
VOLUME	[ "/tmp/qkmj" ]
CMD ["sh", "-c", "echo \"telnet\tstream\ttcp\tnowait\tgames:games\t/usr/sbin/tcpd\t/usr/sbin/telnetd -E '/opt/qkmj/qkmj ${QKMJ_SERVER}'\" > /etc/inetd.conf && inetutils-inetd && nginx -g 'daemon on;' && /opt/qkmj/mjgps ${MJQPS_DAEMON_PORT}"]