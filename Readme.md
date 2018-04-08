
Backend
===

Tested on Ubuntu 16

Install packages from Vagrant file provision part

Setup python env


Setup uwsgi
===
- start emperror at boot (rc.local)
uwsgi --master --die-on-term --emperor /etc/uwsgi

- copy app config /etc/uwsgi

Setup nginx
===

- copy config from ./config/nginx
- remove default config /etc/nginx/sites-available/default

Setup MQTT (Mosquitto)
===

Redis
===

/etc/redis/redis.conf:
dir /var/lib/redis


