from TDStoreTools import StorageManager
import TDFunctions as TDF

class Serialin:

	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
			
		if par.name == "Active":		
			# send status via mqtt only if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "serialin/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))
				
			# op.Serial_in.op("dmxout").par.active = int(par)
			if par == 1:
				op.Logger.Info("[Serial_in]: Serial communication enabled")
			elif par == 0:
				op.Logger.Info("[Serial_in]: Serial communication disabled")
	
		return


