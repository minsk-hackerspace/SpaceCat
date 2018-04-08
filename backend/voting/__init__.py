from flask import request
from flask import Flask
from flask import render_template, send_from_directory
import paho.mqtt.client as mqtt


app = Flask(__name__,  static_url_path='')


import redis
r = redis.StrictRedis(host='localhost', port=6379, db=0)

mqttc = mqtt.Client()
mqttc.connect("127.0.0.1")
mqttc.loop_start()


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
    mqttc.publish("voting/pos", str(val))


@app.route('/img/<path:path>')
def send_js(path):
    return send_from_directory('img', path)

@app.route('/')
def home():
    val = r.get('counter')
    if val is None:
        val = b'0'
    return render_template('home.html', val=val.decode('utf-8'))

@app.route('/status')
def status():
    val = r.get('counter')
    # return "Count:" + val.decode('utf-8') + '  ' + request.remote_addr
    return render_template('status.html', val=val.decode('utf-8'), addr=request.remote_addr )

@app.route('/vote', methods=['GET', 'POST'])
def vote_post():
    if request.method == 'POST':
        if request.form['vote']=="yes":
            r.incr('counter', amount=1)
        else:
            r.decr('counter', amount=1)

        val = r.get('counter')
        put_to_MQTT()

        # return "Hello World!" + val.decode('utf-8')
        return render_template('vote_done.html', count=val)
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
            val = r.get('counter')
            put_to_MQTT()
            return render_template('admin_done.html', count = val)
        else:
            return 'Пароль то не тот!'
    else:
        return render_template('admin.html')




if __name__ == '__main__':
    app.run(host= '0.0.0.0')

