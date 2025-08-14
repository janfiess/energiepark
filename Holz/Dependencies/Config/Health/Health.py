from TDStoreTools import StorageManager
import TDFunctions as TDF

class Health:
	def __init__(self, ownerComp):
		return
	
	def FPStoolow(self):	
		# TouchDesigner 2022.24200 crashes if the MQTT node receives 
		# messages before active
	
		isMqttReady = op.MQTT_in.op('mqttclient').par.active 
		if isMqttReady != 1:
			return
		
		# If FPS too low
		topic_prefix = op.MQTT_in.par.Topicprefix
		
		if(op("null_fps_too_low")[0]) == 1:
			current_fps = int(op("null_fps")[0])
			op.MQTT_out.SendMQTT_retained(topic_prefix + "app/low_fps", current_fps)
			op("timer_when_fps_too_low").par.start.pulse()
			
	def OnParamChanged(self, par):
		if par.name == "Timelineactive":
			me.time.play = int(par)
				 
				
