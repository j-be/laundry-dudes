#!flask/bin/python
import time
import datetime

import domain

from flask import Flask, request, jsonify, abort


data_types = None
app = Flask(__name__)


def _getTimeOfDay(dt):
	return [dt.hour, dt.minute, dt.second]


@app.route('/laundrydude')
def index():
	return app.send_static_file('index.html')

@app.route('/laundrydude/<string:file_name>')
def static_html_proxy(file_name):
	return app.send_static_file(file_name)

@app.route('/laundrydude/css/<path:path>')
def static_css_proxy(path):
	return app.send_static_file('css/' + path)

@app.route('/laundrydude/js/<path:path>')
def static_js_proxy(path):
	return app.send_static_file('js/' + path)

@app.route('/laundrydude/img/<path:path>')
def static_img_proxy(path):
	print path
	return app.send_static_file('img/' + path)

@app.route('/laundrydude/api/data')
def get_data():
	values = {}

	for data_type in data_types.keys():
		domain_cls = data_types[data_type]
		values[data_type] = [(_getTimeOfDay(row.timestamp), row.value)
							 for row in domain_cls.select()]

	return jsonify(values), 200

@app.route('/laundrydude/api/clear')
def clear_db():
	print "Clearing DB..."

	for cls in data_types.values():
		cls.deleteMany('id=id')

	return jsonify({'e': 0}), 200

@app.route('/laundrydude/api/data', methods=['POST'])
def save_data():
	global values

	data_dict = request.form
	print (request.form)

	if not data_dict:
		abort(400)

	for data_type in data_dict.keys():
		value = float(data_dict[data_type].strip())
		data_types[data_type](value=value)

	return jsonify({"e": 0}), 201


if __name__ == '__main__':
	data_types = domain.createDb()
	app.run(host='0.0.0.0', debug=True)
