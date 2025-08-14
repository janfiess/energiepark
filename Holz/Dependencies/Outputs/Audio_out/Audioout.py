import difflib
from TDStoreTools import StorageManager
import TDFunctions as TDF

class Audioout:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):
		
		if par.name == "Mute":
			if int(par) == 1:
				op.Logger.Info("[Audio_out]: Audio muted")
			elif int(par) == 0:
				op.Logger.Info("[Audio_out]: Audio un-muted")

			topic = op.MQTT_in.par.Topicprefix  + "udpout/active"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(op.MQTT_in.par.Topicprefix + "audio/mute", int(par))

		# iwrd aufgerufen in op("chopexec_sendslidervalafterchanged"), nachdem slider von Hand bewegt wurde
		elif par.name == "Audiovolume":
			par = round(float(par),2)
			topic = op.MQTT_in.par.Topicprefix  + "audio/volume"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, par)
				if parent().par.Nextaudiovolume != parent().par.Audiovolume:
					parent().par.Nextaudiovolume = par
				op.Logger.Info(f"[Audio_out]: Audiovolume: {par}")

		elif par.name == "Playavsynctest":
			op.ScenePlayer.PlayScene("avsynctester")	