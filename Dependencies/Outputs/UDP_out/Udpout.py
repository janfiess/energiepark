from TDStoreTools import StorageManager
import TDFunctions as TDF

class Udpout:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
		if par.name == "Active":

			topic = op.MQTT_in.par.Topicprefix  + "udpout/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))


			#op.MQTT_out.SendMQTT_retained(op.MQTT_in.par.Topicprefix + "udpout/active", int(par))
			op.UDP_out.op("udpout").par.active = int(par)
			if par == 1:
				op.Logger.Info("[UDP_out]: UDP out enabled")

			elif par == 0:
				op.Logger.Info("[UDP_out]: UDP out disabled")



		if par.name == "Ipaddress":
			op.UDP_out.op("udpout").par.Ipaddress = str(par)
			op.Logger.Info(f"[UDP_out]: UDP out IP Address: {str(par)}")
			
		if par.name == "Port":
			op.UDP_out.op("udpout").par.Port = int(par)
			op.Logger.Info(f"[UDP_out]: UDP out port: {int(par)}")
		return

	def Send(self, message):
		op('udpout').send(str(message))
		op.Logger.Debug(f"UDP OUT --- Message: {message}")