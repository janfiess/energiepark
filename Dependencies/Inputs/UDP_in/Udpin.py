from TDStoreTools import StorageManager
import TDFunctions as TDF

class Udpin:

	def __init__(self, ownerComp):
		return
		
	def Triggeraction(self, message):
		op.Logger.Debug(f"[UDP_in]: Incoming message: {message}")

	def OnParamChanged(self, par):

		if par.name == "Active":
			op.MQTT_out.SendMQTT_retained( op.MQTT_in.par.Topicprefix + "udpin/active", int(par))
			op.Logger.Debug(f"[UDP_in]: UDP IN ative: {int(par)}")
			
		if par.name == "Testudpoutcommands":
			if par == "en":
				op.UDP_out.Send("e")
			elif par == "fr":
				op.UDP_out.Send("f")
			elif par == "de":
				op.UDP_out.Send("d")