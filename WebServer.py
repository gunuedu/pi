from flask import Flask, render_template

mos = 0
hum1 = 3
hum2 = 4
lux = 0
tmpTarget = 30 #목표온도기준값
humTarget = 40 #목표습도기준값



app = Flask(__name__)

@app.route('/getState')
def get_state():
	return '%s/%s/%s/%s/' % (tmp1, tmp2, hum1, hum2)

@app.route('/tmp1')
def hello_tmp1():
	return '현재온도@%s' % tmp1

@app.route('/tmp2')
def hello_tmp2():
	return '현재온도@%s' % tmp2

@app.route('/tmpUp')
def tmpUp():
	global tmpTarget 
	tmpTarget = tmpTarget+1
	return '목표온도@%s'% tmpTarget

@app.route('/tmpDown')
def tmpDown():
        global tmpTarget
        tmpTarget = tmpTarget-1
        return '목표온도@%s'% tmpTarget

@app.route('/hum1')
def hello_hum1():
	return '현재습도@%s' % hum1

@app.route('/hum2')
def hello_hum2():
	return '현재습도@%s' % hum2

@app.route('/humUp')
def humUp():
	global humTarget
	humTarget = humTarget + 1
	return '목표습도@%s' % humTarget

@app.route('/humDown')
def humDown():
        global humTarget
        humTarget = humTarget - 1
        return '목표습도@%s' % humTarget

@app.route('/lux')
def hello_lux():
	return '현재조도 @%s' % lux

@app.route('/grd')
def hello_grd():
	return '현재토양 @%s' % mos

@app.route('/manual')
def hello_manual():
	return '@0'

@app.route('/auto')
def hello_automatic():
	return '@1'

@app.route('/switchOn')
def switchOn():
	return '@1'

@app.route('/switchOff')
def switchOff():
	return '@0'

@app.route('/fanAuto')
def fanAuto():
	return '@1'

@app.route('/fanManual')
def fanManual():
	return '@0'

@app.route('/heaterAuto')
def heaterAuto():
	return '@1'

@app.route('/heaterManual')
def heaterManual():
	return '@0'

@app.route('/')
def home():
	return render_template("home.html", mosH=mos,tmp1H=tmp1,tmp2H=tmp2,hum1H=hum1,hum2H=hum2,luxH=lux)

@app.route('/<mosIn>/<tmp1In>/<tmp2In>/<hum1In>/<hum2In>/<luxIn>')
def home2(mosIn,tmp1In,tmp2In,hum1In,hum2In,luxIn):
	global mos,tmp1,tmp2,hum1,hum2,lux
	mos = mosIn
	tmp1 = tmp1In
	tmp2 = tmp2In
	hum1 = hum1In
	hum2 = hum2In
	lux = luxIn
	return render_template("change.html",mosH=mosIn,tmp1H=tmp1In,tmp2H=tmp2In,hum1H=hum1In,hum2H=hum2In,luxH=luxIn)

if __name__ == '__main__':
	app.run(host='0.0.0.0', port=80)
