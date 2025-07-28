from TDStoreTools import StorageManager
import TDFunctions as TDF

class Timer:

	def __init__(self, ownerComp):
		return

	def OnParamChanged(self, par):
		if par.name == "Resetalltimers":
			op.Logger.Info(f"[Timer]: Abort all timers")
		elif par.name == "Installationisinuse":
			op("timer_if_installation_in_in_use").par.start.pulse()

		elif par.name == "Timerlength":
			# raise NotImplementedError
			par = int(par)
			op.Logger.Info(f"[Timer]: Changed timer length to {par}s")
			# send status only via mqtt if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "timerlength"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, par)
			
