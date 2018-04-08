
# Backend

Tested on Ubuntu 16

Install packages from Vagrant file provision part

## Setup python env and app

```
cd /opt
git clone https://github.com/minsk-hackerspace/SpaceCat.git

cd /opt/SpaceCat/backend
virtualenv -p python3 venv
source venv/bin/activate
pip install -r requirements.txt

```

## Redis


### /etc/redis/redis.conf:

- dir /var/lib/redis
- supervised systemd

### Create service config

- Get file contents from ./config
- /etc/systemd/system/redis.service

### run redis

```
systemctl enable redis
systemctl start redis
```

## Setup uwsgi

```
cd /opt/SpaceCat
mkdir -p /etc/uwsgi
cp backend/voting.ini /etc/uwsgi/

```

### start emperror at boot as service

- Get file contents from ./config
- /etc/systemd/system/redis.service

```
systemctl enable uwsgi
systemctl start uwsgi
```


### App config
- copy app config /etc/uwsgi
- correct paths in ini

## Setup nginx
```
sudo cp config/server/etc/nginx/sites-available/space_cat.conf /etc/nginx/sites-available/
sudo ln -s /etc/nginx/sites-available/space_cat.conf /etc/nginx/sites-enabled/
rm /etc/nginx/sites-enabled/default
```

## Setup MQTT (Mosquitto)



