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
	

		# Lagesensor
		
		elif(topic == self.topic_prefix + "lagesensor"):  
			op.Solarpanel.op("text_lagesensor").text = payload


		

		# HTTP in

		elif (topic == self.topic_prefix + "httpin/active"): # retained
			if op.HTTP_in.par.Active is not None:	
				if int(op.HTTP_in.par.Active) != int(payload):
					op.HTTP_in.par.Active = int(payload)				
		
		


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



