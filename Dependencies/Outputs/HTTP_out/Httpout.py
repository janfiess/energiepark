from TDStoreTools import StorageManager
import TDFunctions as TDF

class Httpout:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
		if par.name == "Active":
			
			# send status only via mqtt if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "httpout/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, int(par))
				
			op.MQTT_out.SendMQTT_retained( op.MQTT_in.par.Topicprefix + "httpout/active", int(par))
			#op("webclient1").par.active = int(par)
			if par == 1:
				op.Logger.Info("[HTTP_out]: HTTP out enabled")
			elif par == 0:
				op.Logger.Info("[HTTP_out]: HTTP out disabled")
			

			
		elif par.name == "Resetposition":
			domain = "http://172.16.0.1:8085/do_set_manual.html?which=resetPosition&selectedColumn=-1&selectedRow=-1"
			self.sendHTTP(domain)


	def sendHTTP(self, domain):
		webClient = op('webclient1') # Our WebClient DAT
		
		firstRequest = webClient.request(
			domain, # Final endpoint we are querying
			'GET' # Method
		)
