from TDStoreTools import StorageManager
import TDFunctions as TDF

class Lidarin:

	def __init__(self, ownerComp):
		return
	
	def OnParamChanged(self, par):
		if par.name == "Active":		
			# send status via mqtt only if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "lidar/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))
				
		elif par.name == "Mute":
			op.Lidar_in.par.Mute = int(par)
			topic = op.MQTT_in.par.Topicprefix + "lidar/mute"
			op.MQTT_out.SendMQTT_retained(topic, int(par))
			
			if par == 1:
				op.Logger.Info("[Lidar_in]: Lidar muted")   # Aktion wird bereits bei den Custom parameters umgesetzt
			elif par == 0:
				op.Logger.Info("[Lidar_in]: Lidar unmuted")   # Aktion wird bereits bei den Custom parameters umgesetzt
			
		elif par.name == "Ipaddress":
			op.Logger.Info(f"[Lidar_in]: Lidar IP Address: {par}")
			
		elif par.name == "Reactivate":
			op.Logger.Info(f"[Lidar_in]: Reactivate lidar")

