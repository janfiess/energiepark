from TDStoreTools import StorageManager
import TDFunctions as TDF

class ScenePlayer:
	def __init__(self, ownerComp):
		return

	# triggered by op.MQTT_in.op('trigger_by_incoming_mqtt')
	def PlayScene(self, payload):
		payload = str(payload)

		if payload not in self.accepted_payloads_list or "None" in payload:
			return
		
		op.Timer.par.Resetalltimers.pulse()	
		parent().par.Nextreplicatorinstance = self.GetSceneName(payload) # 'footage*'

		op.ScenePlayer.op("timer_fadeoldout_newin").par.start.pulse()


		#print(f"next replicator instance: {parent().par.Nextreplicatorinstance}")

		parent().par.Prevscenemqttpayload = parent().par.Latestscenemqttpayload
		parent().par.Latestscenemqttpayload = payload
		parent().par.Latestscenemqtttime = absTime.seconds

		# save latest scene mqtt payload on broker in order to have it persistent in case the scene change was not initiated over mqtt
			
		if op.MQTT_in.par.Latestmqttpayload != op.ScenePlayer.par.Latestscenemqttpayload  and (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
			topic = op.MQTT_in.par.Topicprefix  + "player/play"
			op.MQTT_out.SendMQTT_retained(topic, payload)
			


	def Play_audio(self, playername):
		print("playing")
		player = op(playername)
		player.par.play = 1
	
		player.par.cuepoint = 0
		player.par.cuepulse.pulse()

		op.Logger.Info(f"[ScenePlayer]: playing now {playername}")

	def Pause_audio(self, playername):
		print("pause")
		player = op(playername)
		player.par.play = 0
		player.par.cuepulse.pulse()
		op.Logger.Info(f"[ScenePlayer]: pausing {playername}")




	def OnParamChanged(self, par):
		if par.name == "Audio1de":
			if par == 1:
				self.Play_audio("Audio1_DE")
			elif par == 0:
				self.Pause_audio("Audio1_DE")

		elif par.name == "Audio1en":
			if par == 1:
				self.Play_audio("Audio1_EN")
			elif par == 0:
				self.Pause_audio("Audio1_EN")

		elif par.name == "Audio2de":
			if par == 1:
				self.Play_audio("Audio2_DE")
			elif par == 0:
				self.Pause_audio("Audio2_DE")

		elif par.name == "Audio2en":
			if par == 1:
				self.Play_audio("Audio2_EN")
			elif par == 0:
				self.Pause_audio("Audio2_EN")
				