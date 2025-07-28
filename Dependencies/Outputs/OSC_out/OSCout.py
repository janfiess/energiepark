from TDStoreTools import StorageManager
import TDFunctions as TDF

class OSCout:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
		if par.name == "Active":	
			topic = op.MQTT_in.par.Topicprefix  + "oscout/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))

			if par == 1:
				op.Logger.Info("[OSC_out]: OSC out enabled")
				
			elif par == 0:
				op.Logger.Info("[OSC_out]: OSC out disabled")

		if par.name == "Ipaddress":
			# op("oscout1").par.Ipaddress = par
			op.Logger.Info(f"[OSC_out]: OSC out IP Address: {par}")
			
		if par.name == "Port":
			# op("oscout1").par.Port = int(par)
			op.Logger.Info(f"[OSC_out]: OSC out port: {par}")
			
	def Send(self, key, value):
		op('oscout').sendOSC(key, [value])
		op.Logger.Debug(f"OSC OUT --- Key: {key}, Value: {value}")
