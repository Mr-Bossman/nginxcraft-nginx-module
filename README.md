Name
====

**nginxcraft-nginx-module** - Nginx module to Proxy Minecraft servers

*This module is not distributed with the Nginx source.* See [the installation instructions](#installation).

Table of Contents
=================

* [Name](#name)
* [Synopsis](#synopsis)
* [Description](#description)
* [Content Handler Directives](#content-handler-directives)
    * [nginxcraft](#nginxcraft)
* [Variables](#variables)
    * [$minecraft_server](#minecraft_server)
* [Installation](#installation)
* [Compatibility](#compatibility)
* [Source Repository](#source-repository)
* [TODO](#todo)


Synopsis
========

[Back to TOC](#table-of-contents)

Description
===========


[Back to TOC](#table-of-contents)

Content Handler Directives
==========================

nginxcraft
----
**syntax:** *nginxcraft \[on;off\]*

**default:** *no*

**context:** *server*

**phase:** *content*
```nginx
	server {
		listen		25565;
		server_name	local.example.com;
		nginxcraft	on;
		proxy_pass	mc.hypixel.com:25565;
	}
```

[Back to TOC](#table-of-contents)

Variables
=========

$minecraft_server
-------------------

This variable holds the minecraft server name.

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
* Port and version variable
* Attribute the readme template
* **Fix my life**
