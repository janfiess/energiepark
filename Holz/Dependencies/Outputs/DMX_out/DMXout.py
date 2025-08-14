from TDStoreTools import StorageManager
import TDFunctions as TDF

class DMXout:

	def __init__(self, ownerComp):
		return

	def OnParamChanged(self, par):
		if par.name == "Active": # retained
			op.DMX_out.op("dmxout").par.active = int(par)
			if par == 1:
				op.Logger.Info("[DMX_out]: DMX out enabled")
			elif par == 0:
				op.Logger.Info("[DMX_out]: DMX out disabled")

			# send status only via mqtt if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "dmxout/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))
			
		if par.name == "Ipaddress":
			op.Logger.Info(f"[DMX_out]: DMX out IP Address: {par}")
			
		if par.name == "Port":
			op.Logger.Info(f"[DMX_out]: DMX out port: {par}")
