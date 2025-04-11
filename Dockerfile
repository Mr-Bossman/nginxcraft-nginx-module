FROM debian:latest
EXPOSE 25565

RUN apt update && \
	apt upgrade -y && \
	apt install -y wget curl tar build-essential libpcre3-dev zlib1g-dev && \
	rm -rf /var/lib/apt/lists/*

RUN mkdir /root/nginx
WORKDIR /root/nginx
ENTRYPOINT ["/root/nginx/build.sh"]
# Don't run as root
# podman build -t nginxcraft .
# podman run --rm -v $(pwd):/root/nginx -it nginxcraft {opt version}
# podman run --rm -v $(pwd):/root/nginx -it --entrypoint /bin/bash nginxcraft {opt version}
