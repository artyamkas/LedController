import socket
from flask import Flask, render_template, request, redirect, jsonify, session
import requests
import os

app = Flask(__name__)
app.secret_key = os.urandom(24)  # Для работы с сессиями

def get_local_ip():
    """Получаем локальный IP адрес компьютера в сети"""
    try:
        # Создаем временный сокет для определения IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))  # Подключаемся к публичному DNS
        local_ip = s.getsockname()[0]
        s.close()
        return local_ip
    except Exception:
        return "127.0.0.1"

# Получаем IP автоматически
LOCAL_IP = get_local_ip()
# Начальное значение IP (может быть переопределено через сессию)
DEFAULT_ESP8266_IP = 'http://192.168.1.100'

def get_esp_ip():
    """Получаем текущий IP из сессии или используем дефолтный"""
    return session.get('esp_ip', DEFAULT_ESP8266_IP)

@app.route('/')
def index():
    return render_template('index.html', current_ip=get_esp_ip())

@app.route('/update_ip', methods=['POST'])
def update_ip():
    new_ip = request.form.get('ip')
    if new_ip:
        # Проверяем валидность IP (базовая проверка)
        if new_ip.startswith(('http://', 'https://')):
            session['esp_ip'] = new_ip
            return jsonify({'status': 'success', 'ip': new_ip})
    return jsonify({'status': 'error'}), 400

@app.route('/power/<state>')
def power(state):
    esp_ip = get_esp_ip()
    if state == 'on':
        requests.get(f'{esp_ip}/on')
    elif state == 'off':
        requests.get(f'{esp_ip}/off')
    return redirect('/')

@app.route('/color', methods=['GET'])
def color():
    esp_ip = get_esp_ip()
    r = request.args.get('r')
    g = request.args.get('g')
    b = request.args.get('b')
    if all([r, g, b]):
        requests.get(f'{esp_ip}/color?r={r}&g={g}&b={b}')
    return redirect('/')

@app.route('/mode', methods=['POST'])
def mode():
    esp_ip = get_esp_ip()
    mode = request.form.get('mode', '0')
    requests.get(f'{esp_ip}/mode?m={mode}')
    return '', 200

if __name__ == '__main__':
    app.run(host=LOCAL_IP, port=5050)