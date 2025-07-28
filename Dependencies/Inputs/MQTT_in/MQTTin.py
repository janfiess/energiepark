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
					
			op.MQTT_in.par.Prevmqtttopic = op.MQTT_in.par.Latestmqtttopic
			op.MQTT_in.par.Prevmqttpayload = op.MQTT_in.par.Latestmqttpayload
			op.MQTT_in.par.Latestmqtttopic = topic
			op.MQTT_in.par.Latestmqttpayload = payload
			op.MQTT_in.par.Latestmqtttime = absTime.seconds
			# op.Logger.MQTT_in(topic, payload)
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
	

		# Player

		elif(topic == self.topic_prefix + "reset"):
			op.Reset.ResetRoutine()
		
		elif(topic == self.topic_prefix + "player"):   # eg. T: laufen/player P: play
			if(payload == "pause"):
				op.ScenePlayer.Pause()
			elif(payload == "continue"):
				op.ScenePlayer.Continue()
			elif(payload == "force_restart"):
				op.ScenePlayer.Restart()
			elif(payload == "blackout"):
				op.Display_out.par.Displayblack = 1
			elif(payload == "next_scene"):
				op.ScenePlayer.par.Nextscene.pulse()
			elif(payload == "prev_scene"):
				op.ScenePlayer.par.Prevscene.pulse()
	
		elif(topic == self.topic_prefix + "player/play"):   # eg. T: [topic_prefix]play P: democontent1   # retained
			if op.ScenePlayer is not None:	
				if str(op.ScenePlayer.par.Latestscenemqttpayload) != str(payload):
					# print(f"Payload to be sent to ScenePlayer: {payload}")
					op.ScenePlayer.PlayScene(payload)
					


		elif(topic == self.topic_prefix + "player/pause"):  
			op.ScenePlayer.par.Pause = int(payload)
		
		elif (topic == self.topic_prefix + "player/fadeintime"): # retained
			if op.ScenePlayer.par.Fadeintime is not None:	
				if float(op.ScenePlayer.par.Fadeintime) != float(payload):
					op.ScenePlayer.par.Fadeintime = float(payload)


			
		elif (topic == self.topic_prefix + "player/fadeouttime"): # retained
			if op.ScenePlayer.par.Fadeouttime is not None:	
				if float(op.ScenePlayer.par.Fadeouttime) != float(payload):
					op.ScenePlayer.par.Fadeouttime = float(payload)
		

		# Lidar_in
			
		elif (topic == self.topic_prefix + "lidar/active"): # retained
			if op.Lidar_in.par.Active is not None:	
				if int(op.Lidar_in.par.Active) != int(payload):
					op.Lidar_in.par.Active = int(payload)


		elif (topic == self.topic_prefix + "lidar/mute"): # retained
			if op.Lidar_in.par.Mute is not None:	
				if int(op.Lidar_in.par.Mute) != int(payload):
					op.Lidar_in.par.Mute = int(payload)



			
		elif (topic == self.topic_prefix + "lidar/reactivate"):
			op.Lidar_in.par.Reactivatelidar.pulse()


		# cms in
			
		elif (topic == self.topic_prefix + "cms/use_fake_contentjson"): # retained
			if op.CMS_in.par.Fakecontentjson is not None:		
				if int(op.CMS_in.par.Fakecontentjson) != int(payload):
					op.CMS_in.par.Fakecontentjson = int(payload)
	
			
		# Serial_in
			
		elif (topic == self.topic_prefix + "serialin/active"):
			if op.Serial_in.par.Active is not None:		
				if int(op.Serial_in.par.Active) != int(payload):
					op.Serial_in.par.Active = int(payload)


		# UDP_in
			
		elif (topic == self.topic_prefix + "udpin/active"): # retained
			if op.UDP_in.par.Active is not None:		
				if int(op.UDP_in.par.Active) != int(payload):
					op.UDP_in.par.Active = int(payload)
					
		
		# OSC_in
			
		elif (topic == self.topic_prefix + "oscin/active"): # retained
			if op.OSC_in.par.Active is not None:		
				if int(op.OSC_in.par.Active) != int(payload):
					op.OSC_in.par.Active = int(payload)
		
	
		# Language_in
		
		elif(topic == self.topic_prefix + "language"): # retained
			if op.Language.par.Selectedlanguage is not None:	

				# print(f"op.Language.par.Selectedlanguage: {op.Language.par.Selectedlanguage}, str(payload): {str(payload)}")		
				if op.Language.par.Activelanguage != str(payload):
					op.Logger.Info(f"[MQTT_in]: INSIDE: op.Language.par.Selectedlanguage: {op.Language.par.Selectedlanguage}, str(payload): {str(payload)}")		
				
					if (payload == "de"):
						op.Language.par.Selectedlanguage = 0

					elif (payload == "en"):
						op.Language.par.Selectedlanguage = 1

					elif (payload == "fr"):
						op.Language.par.Selectedlanguage = 2


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
				
		# OSC_out
	
		elif(topic == self.topic_prefix + "oscout/active"): # retained
			if op.OSC_out.par.Active is not None:		
				if int(op.OSC_out.par.Active) != int(payload):
					op.OSC_out.par.Active = int(payload)

		# udp_out

		elif (topic == self.topic_prefix + "udpout/active"): # retained
			if op.UDP_out.par.Active is not None:		
				if int(op.UDP_out.par.Active) != int(payload):
					op.UDP_out.par.Active = int(payload)
					
		# Audio_out
				
		elif (topic == self.topic_prefix + "audio/volume"): # retained
			payload = round(float(payload), 2)
			if op.Audio_out.par.Audiovolume is not None:	
				if round(float(op.Audio_out.par.Audiovolume), 2) != payload:
					op.Audio_out.par.Audiovolume = float(payload)
					op.Audio_out.par.Nextaudiovolume = float(payload)

		elif (topic == self.topic_prefix + "audio/mute"): # retained
			if op.Audio_out.par.Mute is not None:	
				op.Audio_out.par.Mute = int(payload)

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

		# http out

		elif (topic == self.topic_prefix + "httpout/active"): # retained
			if op.HTTP_out.par.Active is not None:
				if int(op.HTTP_out.par.Active) != int(payload):
					op.HTTP_out.par.Active = int(payload)

		# timer length
					
		elif (topic == self.topic_prefix + "timer/timerlength"): # retained
			if op.Timer.par.Timerlength is not None:	
				if int(op.Timer.par.Timerlength) != int(payload):
					op.Timer.par.Timerlength = float(payload)


