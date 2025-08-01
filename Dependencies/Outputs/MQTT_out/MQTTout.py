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
		
		

		elif par.name == "Playaudio1de":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio1de", "play")

		elif par.name == "Pauseaudio1de":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio1de", "pause")

		elif par.name == "Playaudio1en":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio1en", "play")

		elif par.name == "Pauseaudio1en":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio1en", "pause")
			
		elif par.name == "Playaudio2de":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio2de", "play")

		elif par.name == "Pauseaudio2de":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio2de", "pause")
			
		elif par.name == "Playaudio2en":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio2en", "play")

		elif par.name == "Pauseaudio2en":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/audio2en", "pause")
				
		elif par.name == "Reset":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "reset",  1)
			
			
			
		# outputs
x
		elif par.name == "Reactivatedisplay":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "display",  "reactivate")

		elif par.name == "Changedisplaybrightness":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "display/brightness",  round( random.uniform(0,1), 2))


		elif par.name == "Changeaudiovolume":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "audio/volume",  round( random.uniform(0,1), 2))

		elif par.name == "Dmxoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "dmxout/active",  int(par))