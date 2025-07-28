from TDStoreTools import StorageManager
import TDFunctions as TDF

class Udpin:

	def __init__(self, ownerComp):
		return
		
	def Triggeraction(self, key, value):
		if key == "/testosc":
			print("Test erfolgreich")

	def OnParamChanged(self, par):

		if par.name == "Active":
			op.MQTT_out.SendMQTT_retained( op.MQTT_in.par.Topicprefix + "udpin/active", int(par))
			op.Logger.Debug(f"[OSC_in]: OSC IN ative: {int(par)}")
			

			
			