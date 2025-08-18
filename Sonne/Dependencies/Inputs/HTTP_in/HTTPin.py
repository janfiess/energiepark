from TDStoreTools import StorageManager
import TDFunctions as TDF

import os

class HTTPin:

	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
		
		if par.name == "Active":
			# send status via mqtt only if it didn't come from the mqtt broker
			# topic = op.MQTT_in.par.Topicprefix  + "httpin/active"
			# if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
			# 	op.MQTT_out.SendMQTT_retained(topic, int(par))
				
			op.Logger.Info(f"[HTTP_in]: active: {int(par)}")


		elif par.name == "Shutdownpc":
			if parent().par.Active == 1:
				op.Logger.Info(f"[HTTP_in]: Shutdown PC")
				op("webclient").par.request.pulse()
			
		elif par.name == "Cancelshutdownpc":
			op.Logger.Info(f"[HTTP_in]: Cancel Shutdown PC")
			os.system('shutdown -a')
			
	

