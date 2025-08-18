from TDStoreTools import StorageManager
import TDFunctions as TDF

class MQTTin:
	def __init__(self, ownerComp):
		self.topic_prefix = parent().par.Topicprefix
		
	def OnParamChanged(self, par):
		
		if par.name == "Active":
			if par == 1:
				op.Logger.Info("[MQTT_in]: Activated MQTT")
			elif par == 0:
				op.Logger.Info("[MQTT_in]: Deactivated MQTT")
			
		elif par.name == "Reconnect":
			op.Logger.Info(f"[MQTT_in]: Reconnecting to MQTT broker")
			
		elif par.name == "Broker":
			op.Logger.Info(f"[MQTT_in]: Selected MQTT brokerchanged to: {par}")

		elif par.name == "Topicprefix":
			op.Logger.Info(f"[MQTT_in]: mqtt_topic_prefix changed to: {par}")
			op.MQTT_in.par.Topicprefix = str(par)



	def Process_mqtt_action(self, topic, payload, retained):
		if len(payload):
			payload = payload.decode("utf-8")   # convert byte to string

			op.Logger.Info(f"[MQTT_in]: topic: {topic}   |   payload: {payload}")
			self.Triggeraction(topic, payload)

	def Triggeraction(self, topic, payload):

		# APP		
				
		if(topic == self.topic_prefix + "app/active"):
			op.Health.par.Timelineactive = int(payload)


		elif(topic == self.topic_prefix + "app"):
			if(payload == "shutdownpc"):
				op.Closing.ShutDownPC()
			elif(payload == "show_display_ui"):
				op.Display_out.par.Showui.pulse()
	

		# AUdio Player Kopfhörerstationen

		elif(topic == self.topic_prefix + "reset"):
			op.Reset.ResetRoutine()

		elif(topic == self.topic_prefix + "player/audio1de"):
			if(payload == "play"):
				op.Audiostationen.Play_audio("Audio1_DE")
			elif(payload == "pause"):
				op.Audiostationen.Pause_audio("Audio1_DE")
		
		elif(topic == self.topic_prefix + "player/audio1en"):
			if(payload == "play"):
				op.Audiostationen.Play_audio("Audio1_EN")
			elif(payload == "pause"):
				op.Audiostationen.Pause_audio("Audio1_EN")

		elif(topic == self.topic_prefix + "player/audio2de"): 
			if(payload == "play"):
				op.Audiostationen.Play_audio("Audio2_DE")
			elif(payload == "pause"):
				op.Audiostationen.Pause_audio("Audio2_DE")

		elif(topic == self.topic_prefix + "player/audio2en"): 
			if(payload == "play"):
				op.Audiostationen.Play_audio("Audio2_EN")
			elif(payload == "pause"):
				op.Audiostationen.Pause_audio("Audio2_EN")
				
		# RFID-Stationen
		
		elif(topic == self.topic_prefix + "rfid"):  
			op.RFIDScanner.Lookup_rfid(payload)


		

		# HTTP in

		elif (topic == self.topic_prefix + "httpin/active"): # retained
			if op.HTTP_in.par.Active is not None:	
				if int(op.HTTP_in.par.Active) != int(payload):
					op.HTTP_in.par.Active = int(payload)				
		
		


		# DMX_out

		elif(topic == self.topic_prefix + "dmxout/active"): # retained

			if op.DMX_out.par.Active is not None:		
				if int(op.DMX_out.par.Active) != int(payload):
					op.DMX_out.par.Active = int(payload)
				

					
		# Audio_out
				
		elif (topic == self.topic_prefix + "audio/volume"): # retained
			payload = round(float(payload), 2)
			if op.Audio_out.par.Audiovolume is not None:	
				if round(float(op.Audio_out.par.Audiovolume), 2) != payload:
					op.Audio_out.par.Audiovolume = float(payload)
					op.Audio_out.par.Nextaudiovolume = float(payload)


		# Display_out
		
		elif(topic == self.topic_prefix + "display"):
			if(payload == "reactivate"):
				op.Display_out.par.Reactivatedisplay.pulse()
				
		elif(topic == self.topic_prefix + "display/black"):
			op.Display_out.par.Displayblack = int(payload)

		elif(topic == self.topic_prefix + "display/brightness"): # retained
			payload = round(float(payload), 2)
			if op.Display_out.par.Brightness is not None:	
				if round(float(op.Display_out.par.Brightness),2) != payload:
					op.Display_out.par.Brightness = payload
					op.Display_out.par.Nextbrightness = payload



