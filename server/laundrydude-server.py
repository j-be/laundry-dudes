#!flask/bin/python
import datetime
import time
import smtplib

from email.mime.text import MIMEText

import domain

from sqlobject import SQLObjectNotFound
from flask import Flask, request, jsonify, abort

LED_THRESHOLD = 600

data_types = None
app = Flask(__name__)

washer_state = None

def _getTimeOfDay(dt):
	return "%02d.%02d, %02d:%02d" % (dt.day, dt.month, dt.hour, dt.minute)

def changeState(new_state):
	global washer_state
	if new_state == washer_state.value:
		return;

	washer_state = domain.State(value=new_state)

	if new_state == 3:
		user = getCurrentUser()
		if user is not None:
			sendMail(user.email, "[LaundryDude] Washer almost done...",
				"Hi %s,\n\nthe machine is spin-drying - your laundry should be "
				"done soon.\n\nKind regards,\nyour LaundryDudes" % user.name)
	if new_state == 4:
		user = getCurrentUser()
		if user is not None:
			sendMail(user.email, "[LaundryDude] Washer done!",
				"Hi %s,\n\nyour laundry is done!\n\nKind regards,\n"
				"your LaundryDudes" % user.name)

def sendMail(to,subject,text):
	fromAddress = 'laundry.dude.notificator@gmail.com'

	msg = MIMEText(text)
	msg['Subject'] = subject
	msg['From'] = fromAddress
	msg['To'] = to

	# Send the mail
	server = smtplib.SMTP('smtp.gmail.com:587')
	server.starttls()
	server.login(fromAddress, password)
	server.sendmail(fromAddress, to, msg.as_string())
	server.quit()

def getNextReservation():
	try:
		now = time.time()
		select_clause = 'start > %s' % now
		reservation = domain.Reservation.select(select_clause).orderBy('start').limit(1)
		reservation = reservation.getOne()
		return reservation
	except SQLObjectNotFound:
		return None

@app.route('/laundrydude/')
def index():
	return app.send_static_file('index.html')

@app.route('/laundrydude/<path:file_name>')
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

def getCurrentUser():
	try:
		rfid_tag = domain.RfidCard.select().orderBy('-id').limit(1).getOne().value
		return domain.User.select('rfid == "' + rfid_tag + '"').getOne()
	except SQLObjectNotFound:
		return None

@app.route('/laundrydude/api/last-data')
def get_last_data():
	values = {}

	for data_type in ['h', 's', 't']:
		domain_cls = data_types[data_type]
		last_row = domain_cls.select().orderBy('-id').limit(1).getOne()
		values[data_type] = (_getTimeOfDay(last_row.timestamp), last_row.value)

	user = getCurrentUser()
	if user:
		values['u'] = user.name

	reservation = getNextReservation()
	if reservation:
		reservation_info = {}
		reservation_info['user'] = reservation.user
		reservation_info['start'] = _getTimeOfDay(
			datetime.datetime.fromtimestamp(reservation.start))
		reservation_info['startTs'] = reservation.start
		values['r'] = reservation_info

	return jsonify(values), 200

@app.route('/laundrydude/api/blocker')
def get_blocker_state():
	return "b=0", 200

@app.route('/laundrydude/api/clear')
def clear_db():
	print "Clearing DB..."

	for cls in data_types.values():
		cls.deleteMany('id=id')

	return jsonify({'e': 0}), 200

def handleStateChange(key, value):
	if key == "l":
		if value > LED_THRESHOLD and washer_state.value == 0:
			changeState(1)
		if value <= LED_THRESHOLD and washer_state.value == 1:
			changeState(2)
		if value > LED_THRESHOLD and washer_state.value > 1 and washer_state != 4:
			changeState(4)
		if value <= LED_THRESHOLD and washer_state.value == 4:
			changeState(0)
	elif key == "a":
		if abs(value) - 1190 > 300 and washer_state.value == 2:
			changeState(3)

@app.route('/laundrydude/api/data', methods=['POST'])
def save_data():
	global washer_state

	data_dict = request.form

	if not data_dict:
		abort(400)

	for data_type in data_dict.keys():
		if data_type == "r":
			value = data_dict[data_type].strip()
		else:
			value = float(data_dict[data_type].strip())
		data_types[data_type](value=value)
		handleStateChange(data_type, value)

	return jsonify({"e": 0}), 201

@app.route('/laundrydude/api/reservation', methods=['POST'])
def save_reservation():
	start_time = time.mktime(time.strptime(
			request.json['start'], "%Y-%m-%dT%H:%M:%S.000Z"))
	print domain.Reservation(
		user=request.json['title'],
		start=start_time + 3600)
	return jsonify({"e": 0}), 201

@app.route('/laundrydude/api/reservation', methods=['GET'])
def get_reservations():
	return jsonify(domain.sqlresultToDictList(domain.Reservation.select())), 200


if __name__ == '__main__':
	data_types = domain.createDb()
	washer_state = domain.State(value=0)
	sendMail('juriberlanda@hotmail.com', '[LaundryDudes] Started', '')
	app.run(host='0.0.0.0', debug=True)
