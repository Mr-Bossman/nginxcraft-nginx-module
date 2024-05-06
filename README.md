Name
====

**nginxcraft-nginx-module** - Nginx module to reverse proxy Minecraft servers on layer 7

*This module is not distributed with the Nginx source.* See [the installation instructions](#installation).

Table of Contents
=================

* [Name](#name)
* [Synopsis](#synopsis)
* [Description](#description)
* [Content Handler Directives](#content-handler-directives)
    * [nginxcraft](#nginxcraft)
    * [nginxcraft_return](#nginxcraft_return)
* [Variables](#variables)
    * [$minecraft_server](#minecraft_server)
    * [$minecraft_version](#minecraft_version)
    * [$minecraft_port](#minecraft_port)
* [Installation](#installation)
* [Compatibility](#compatibility)
* [Source Repository](#source-repository)
* [TODO](#todo)


Synopsis
========

```nginx

stream {
	log_format	main	'$remote_addr "$server_name" $minecraft_server $new_server "$minecraft_port" $minecraft_version';
	access_log	logs/access.log	main;
	error_log	logs/debug.log	debug;

	upstream google {
		server	google.com:80;
	}

	map $minecraft_server $new_server {
		# Send to "disconnect" server if not in list
		default	unix:/tmp/nginx_disconnect.sock;
		""		google; # for when packet doesn't match a minecraft server
	}

	server {
		# Hacky way to map to another server
		listen				unix:/tmp/nginx_disconnect.sock;
		server_name			disconnect;
		nginxcraft_return	"{'color':'red','text':'You can not connect from: $minecraft_server'}";
	}

	server {
		# Will match if no other server matches
		listen		25565 default_server;
		server_name	_;
		proxy_pass	$new_server;
	}

	server {
		# Will match if url matches because nginxcraft is on
		listen		25565;
		server_name	local.example.com;
		nginxcraft	on;
		proxy_pass	mc.hypixel.net:25565;
	}

	server {
		# Will not match because nginxcraft is off
		listen		25565;
		server_name	local.sample.com;
		proxy_pass	mc.hypixel.net:25565;
	}
}
```

[Back to TOC](#table-of-contents)

Description
===========

**Important notes**:

1. Minecraft has compressed format, documented [here](https://wiki.vg/Protocol#With_compression),
	but is [optionally requested](https://wiki.vg/Protocol#Disconnect_.28login.29) by the server, so it is unnecessary to consider.

[Back to TOC](#table-of-contents)

Content Handler Directives
==========================

nginxcraft
----
**syntax:** *nginxcraft \[on;off\]*

**default:** *no*

**context:** *server*

**phase:** *preread*

```nginx
	server {
		listen		25565;
		server_name	local.example.com;
		nginxcraft	on;
		proxy_pass	mc.hypixel.com:25565;
	}
```

[Back to TOC](#table-of-contents)

nginxcraft_return
----
**syntax:** *nginxcraft_return  &lt;string&gt;*

**default:** *no*

**context:** *server*

**phase:** *content*

Sends string in to client encoded in Minecraft's [Disconnect Packet](https://wiki.vg/Protocol#Disconnect_.28login.29) format
then terminates the connection.

```nginx
	server {
		listen		25565;
		server_name	local.example.com;
		nginxcraft_return	"{'text':'You can not connect from: $minecraft_server'}";
	}
```

[Back to TOC](#table-of-contents)

Variables
=========

$minecraft_server
-------------------

This variable holds the Minecraft server name.

[Back to TOC](#table-of-contents)

$minecraft_version
-------------------

This variable holds the Minecraft client verison.

[List of protocol version numbers](https://wiki.vg/Protocol_version_numbers)

[Back to TOC](#table-of-contents)

$minecraft_port
-------------------

This variable holds the port used to connect in the handshake packet.

[Handshake packet reference](https://wiki.vg/Protocol#Handshake)

[Back to TOC](#table-of-contents)

Installation
============

Grab the nginx source code from [nginx.org](http://nginx.org/), for example,
the version 1.25.5 (see [nginx compatibility](#compatibility)), and then build the source with this module:

```bash

 $ wget 'http://nginx.org/download/nginx-1.25.5.tar.gz'
 $ tar -xzvf nginx-1.25.5.tar.gz
 $ cd nginx-1.25.5/
c
 # Here we assume you would install you nginx under /usr/local/nginx/.
 $ ./configure --prefix=/usr/local/nginx/ --with-stream \
     	 --add-module=/path/to/nginxcraft-nginx-module

 $ make -j$(nproc)
 $ make install
```

Starting from NGINX 1.9.11, you can also compile this module as a dynamic module, by using the `--add-dynamic-module=PATH` option instead of `--add-module=PATH` on the
`./configure` command line above.
If Nginx was build with `--with-compat` you will not need to specify the exact same compilation
options to ./configure that the main NGINX binary was compiled with. You can check with `nginx -V 2>&1 | grep --color=always -- --with-compat`.
To make just the module run `make modules`.
Then you can explicitly load the module in your `nginx.conf` via the [load_module](http://nginx.org/en/docs/ngx_core_module.html#load_module)
directive, for example,

```nginx
load_module /path/to/modules/ngx_stream_nginxcraft_module.so;
```

[Back to TOC](#table-of-contents)

Compatibility
=============

The following versions of Nginx should work with this module:

* **1.25.5**                       (last tested: 1.25.5)

NGINX versions older than 1.25.5 will *not* work due to the lack of support for multiple "stream" server directives.

[Back to TOC](#table-of-contents)

Source Repository
=================

Available on github at [Mr-Bossman/nginxcraft-nginx-module](https://github.com/Mr-Bossman/nginxcraft-nginx-module).

[Back to TOC](#table-of-contents)

TODO
====
* Finish [Synopsis](#synopsis) and [Description](#description)
* Should we be using PREREAD phase?
* Attribute the readme template
* Fix ngx_str_snprintf
* **Fix my life**
