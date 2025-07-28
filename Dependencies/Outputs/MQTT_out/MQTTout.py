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
			
		elif par.name == "Fakecontentjson":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "cms/use_fake_contentjson", int(par))
			
		elif par.name == "Lidarmuted":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "lidar/mute", int(par))

		elif par.name == "Udpinactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "udpin/active", int(par))
			
		elif par.name == "Oscinactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "oscin/active", int(par))
			
		elif par.name == "Lidaractive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "lidar/active", int(par))

		elif par.name == "Httpinactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "httpin/active", int(par))

			
		# player
		
		elif par.name == "Forcerestartvideo":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player", "force_restart")
		
		elif par.name == "Changefadeintime":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/fadeintime", int(par))
			
		elif par.name == "Changefadeouttime":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/fadeouttime", int(par))
			
		elif par.name == "Timelinepaused":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "app/active", int(par))

		elif par.name == "Moviefileplaying":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player/pause", int(par))

		elif par.name == "Languageen":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "language", "en")

		elif par.name == "Languagede":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "language", "de")
		
		elif par.name == "Languagefr":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "language", "fr")

		elif par.name == "Timerlength":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "timer/timerlength", random.randint(3, 30))

		elif par.name == "Prevscene":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player",  "prev_scene")

		elif par.name == "Nextscene":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "player",  "next_scene")

		elif par.name == "Playdemocontent1":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/play", "democontent1")

		elif par.name == "Playdemocontent2":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/play", "democontent2")
			
		elif par.name == "Playdemocontent3":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/play", "democontent3")
			
		elif par.name == "Playavsynctester":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/play", "avsynctester")
			
		elif par.name == "Playgenerativecontent1":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "player/play", "generative1")
		
		elif par.name == "Reset":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "reset",  1)
			
			
			
		# outputs

		elif par.name == "Reactivatedisplay":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "display",  "reactivate")

		elif par.name == "Changedisplaybrightness":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "display/brightness",  round( random.uniform(0,1), 2))

		elif par.name == "Udpoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "udpout/active",  int(par))

		elif par.name == "Displayblackedout":
			op.MQTT_out.SendMQTT_notretained(self.topic_prefix + "display/black",  int(par))

		elif par.name == "Serialoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "serialout/active",  int(par))

		elif par.name == "Audiomuted":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "audio/mute",  int(par))

		elif par.name == "Changeaudiovolume":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "audio/volume",  round( random.uniform(0,1), 2))

		elif par.name == "Oscoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "oscout/active",  int(par))

		elif par.name == "Dmxoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "dmxout/active",  int(par))

		elif par.name == "Httpoutactive":
			op.MQTT_out.SendMQTT_retained(self.topic_prefix + "httpout/active",  int(par))

