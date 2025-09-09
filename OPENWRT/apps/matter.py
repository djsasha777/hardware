from flask import Flask, jsonify
from circuitmatter import MatterController, MatterDevice

app = Flask(__name__)

controller = MatterController()
device = MatterDevice(controller, node_id=0x1234)  # Замените на реальный node_id

@app.route('/up', methods=['GET'])
def up():
    try:
        device.on_off_cluster.on()
        return jsonify({"status": "success", "message": "Команда включения отправлена"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

@app.route('/down', methods=['GET'])
def down():
    try:
        device.on_off_cluster.off()
        return jsonify({"status": "success", "message": "Команда выключения отправлена"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
