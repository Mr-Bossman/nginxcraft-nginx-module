FROM debian:latest
EXPOSE 25565

RUN apt update && \
	apt upgrade -y && \
	apt install -y wget tar build-essential libpcre3-dev zlib1g-dev && \
	rm -rf /var/lib/apt/lists/*

RUN echo '#!/bin/bash\n\
set -e \n\
wget http://nginx.org/download/nginx-$1.tar.gz -O nginx.tar.gz \n\
mkdir -p nginx \n\
tar -xzvf nginx.tar.gz --strip-components=1 -C nginx \n\
rm nginx.tar.gz \n\
chown -R root:root nginx \n\
cd nginx \n\
./configure --prefix=/usr/local/nginx/ --with-compat --with-debug --with-stream \
--add-module=../ --add-dynamic-module=../ \n\
make -j$(nproc) \n\
make install \n\
cp objs/ngx_stream_nginxcraft_module.so /root/nginx/ \n' \
	> /root/build.sh && chmod +x /root/build.sh

RUN mkdir /root/nginx
WORKDIR /root/nginx
ENTRYPOINT ["/root/build.sh"]
CMD ["1.25.5"]
# Don't run as root
# podman build -t nginxcraft .
# podman run --rm -v $(pwd):/root/nginx -it nginxcraft {opt version}
# podman run --rm -v $(pwd):/root/nginx -it --entrypoint /bin/bash nginxcraft {opt version}
