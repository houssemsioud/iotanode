import paho.mqtt.client as paho
import simplejson as json
from influxdb import InfluxDBClient

#Setup some constants with InfluxDB Host and Database name
INFLUXDB_HOST = 'localhost'
INFLUXDB1_NAME = 'temperature_data'
INFLUXDB2_NAME = 'humidity_data'
INFLUXDB3_NAME = 'occupancy_data'
INFLUXDB4_NAME = 'light_data'

#declaring DB clients
db1client = InfluxDBClient(INFLUXDB_HOST,'8086','','',INFLUXDB1_NAME)
db2client = InfluxDBClient(INFLUXDB_HOST,'8086','','',INFLUXDB2_NAME)
db3client = InfluxDBClient(INFLUXDB_HOST,'8086','','',INFLUXDB3_NAME)
db4client = InfluxDBClient(INFLUXDB_HOST,'8086','','',INFLUXDB4_NAME)


#Query Existing Values of temerature
result = db1client.query('SELECT * FROM temperature')
points = list(result.get_points(measurement='temperature'))
for point in points: 
   print('station = ',point['station'],'value = ',point['value'])


def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed: "+str(mid)+" "+str(granted_qos))
 
def on_message(client, userdata, msg):

	d=json.loads(str(msg.payload))
	#a=float(d["fields"]["value"])
	json_data=[d]
	print(json_data)
	if d["measurement"] == "temperature":
		bResult = db1client.write_points(json_data)
	elif d["measurement"] == "Humidity":
		bResult = db2client.write_points(json_data)
	elif d["measurement"] == "occupancy":
		bResult = db3client.write_points(json_data)
	elif d["measurement"] == "brightness":
		bResult = db4client.write_points(json_data)


  
client = paho.Client()
client.on_subscribe = on_subscribe
client.on_message = on_message
client.connect('broker.mqttdashboard.com', 1883)
client.subscribe('0079/#', qos=0) # subscribe to all topics beginning with 0079/

client.loop_forever()
