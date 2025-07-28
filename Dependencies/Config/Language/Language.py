from TDStoreTools import StorageManager
import TDFunctions as TDF

class Language:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):

		if par.name == "Selectedlanguage":	
			# op.Logger.Info("[Language]: before changing language")
			topic = op.MQTT_in.par.Topicprefix  + "language"
			if topic != op.Language.par.Activelanguage  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				self.ChangeLanguage()
				# print("change lang2")


	def ChangeLanguage(self):
		# timestamp zwischenspeichern, damit bei Sprschwechsel der cuepoint von ScenePlayer.py nicht auf 0 gesetzt wird.
		op.ScenePlayer.par.Savedcuepoint = op.ScenePlayer.op("videoinfo1")["index_fraction"]
		

		selected_language = str(op.Language.par.Selectedlanguage)

		if op.Language.par.Activelanguage != selected_language:
			op.Language.par.Activelanguage = selected_language

			op.Logger.Info(f"[Language]: Changing language to {selected_language} now")	

			# nur an broker senden, falls die nachricht nicht sowieso schon vom broker kommt
			topic = op.MQTT_in.par.Topicprefix  + "language" 
			payload = op.Language.par.Activelanguage

			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, payload)
			
