from flask import request
from flask import Flask
from flask import render_template, send_from_directory
import paho.mqtt.client as mqtt
from paho.mqtt.publish import single
# from flask_mqtt import Mqtt

app = Flask(__name__,  static_url_path='')

# mqttc = mqtt.Client()

# app.config['MQTT_BROKER_URL'] = '127.0.0.1'
# app.config['MQTT_BROKER_PORT'] = 1883
# app.config['MQTT_USERNAME'] = ''
# app.config['MQTT_PASSWORD'] = ''
# app.config['MQTT_REFRESH_TIME'] = 1.0  # refresh time in seconds
# mqttc = Mqtt(app)

# @mqttc.on_publish()
# def handle_publish(client, userdata, mid):
    # print('Published message with mid {}.' .format(mid))

import redis
r = redis.StrictRedis(host='localhost', port=6379, db=0)

def get_decoded(keyname):
    val = r.get(keyname)
    if val is not None:
        val = val.decode('utf-8')
    return val


def put_to_MQTT():
    val = r.get('counter')
    max_votes = r.get('max_votes')

    if val is None:
        val = '0'

    if max_votes is None:
        max_votes = '1'

    val = float(val)
    max_votes = float(max_votes)
    res = int(val/max_votes*10000)
    res = max(0, min(res,10000))
    single("voting/pos", payload=res, qos=0, retain=True)
    # pr = mqttc.publish("voting/pos", payload=res, retain=True)
    # print(pr)


@app.route('/img/<path:path>')
def send_js(path):
    return send_from_directory('img', path)

@app.route('/')
def home():
    val = get_decoded('counter')
    max_votes = get_decoded('max_votes')
    return render_template('home.html', val=val, max_votes=max_votes)

@app.route('/status')
def status():
    val = get_decoded('counter')
    max_votes = get_decoded('max_votes')
    # return "Count:" + val.decode('utf-8') + '  ' + request.remote_addr
    return render_template('status.html', max_votes=max_votes, val=val, addr=request.remote_addr )

@app.route('/vote', methods=['GET', 'POST'])
def vote_post():
    if request.method == 'POST':
        if request.form['vote']=="yes":
            r.incr('counter', amount=1)
        else:
            r.decr('counter', amount=1)

        val = get_decoded('counter')
        max_votes = get_decoded('max_votes')
        put_to_MQTT()

        # return "Hello World!" + val.decode('utf-8')
        return render_template('vote_done.html', count=val, max_votes=max_votes)
    else:
        return render_template('vote.html')


@app.route('/admin', methods=['GET', 'POST'])
def admin_post():
    if request.method == 'POST':
        val = r.get('counter')
        password = request.form['password']
        amount = request.form['max_votes']

        if password == "go":
            r.set('counter', 0)
            r.set('max_votes', amount)
            put_to_MQTT()
            val = get_decoded('counter')
            return render_template('admin_done.html', count = val)
        else:
            return 'Пароль то не тот!'
    else:
        max_votes = get_decoded('max_votes')
        return render_template('admin.html', max_votes=max_votes)




if __name__ == '__main__':
    app.run(host= '0.0.0.0')

