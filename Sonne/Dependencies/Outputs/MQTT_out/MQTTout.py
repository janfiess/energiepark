import TDFunctions as TDF
import random

class MQTTout:
	
	def __init__(self, ownerComp):
		self.topic_prefix = op.MQTT_in.par.Topicprefix
		self.mqttclient = op.MQTT_in.op('mqttclient')

	def SendMQTT_retained(self, topic, payload):
		op.Logger.Debug(f"[MQTT_out]: MQTT OUT (retained) --- Topic: {topic} - Payload: {payload}")
		
		payload = bytes(str(payload), 'utf-8')
		self.mqttclient.publish(topic, payload, qos=2, retain = True)	
	
	def SendMQTT_notretained(self, topic, payload):
		op.Logger.Debug(f"[MQTT_out]: MQTT OUT (not retained) --- Topic: {topic} - Payload: {payload}")
		
		payload = bytes(str(payload), 'utf-8')
		self.mqttclient.publish(topic, payload, qos=2, retain = False)
		
	def OnParamChanged(self, par):
		
		# inputs

		if par.name == "Serialinactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "serialin/active", int(par))
			

			
		# player
		
		

		elif par.name == "Lagebeispiel1":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "lagesensor", "x:-15.94,z:3.79")
		elif par.name == "Lagebeispiel2":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "lagesensor", "x:-25,z:44")
		elif par.name == "Lagebeispiel3":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "lagesensor", "x:0,z:0")

		

			
			
			
		# outputs

		elif par.name == "Dmxoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "dmxout/active",  int(par))