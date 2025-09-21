from TDStoreTools import StorageManager
import TDFunctions as TDF

class Audiostationen:
	def __init__(self, ownerComp):
		return

	# triggered by op.MQTT_in.op('trigger_by_incoming_mqtt')
	def PlayScene(self, payload):
		payload = str(payload)

		if payload not in self.accepted_payloads_list or "None" in payload:
			return

	def Play_audio(self, playername):   # z. B. op.Audiostationen.Pause_audio("Audio1_DE")
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
		#player.par.cuepulse.pulse()
		# op.MQTT_out.SendMQTT_notretained(op.MQTT_in.par.Topicprefix + "ledring/state/1", "leds_idle")
		op.Logger.Info(f"[ScenePlayer]: pausing {playername}")




	def OnParamChanged(self, par):
		if par.name == "Audio1":
			if par == 1:
				self.Play_audio("Audio1")
			elif par == 0:
				self.Pause_audio("Audio1")

		elif par.name == "Audio2":
			if par == 1:
				self.Play_audio("Audio2")
			elif par == 0:
				self.Pause_audio("Audio2")

		elif par.name == "Audio3":
			if par == 1:
				self.Play_audio("Audio3")
			elif par == 0:
				self.Pause_audio("Audio3")

		elif par.name == "Audio4":
			if par == 1:
				self.Play_audio("Audio4")
			elif par == 0:
				self.Pause_audio("Audio4")

		elif par.name == "Audio5":
			if par == 1:
				self.Play_audio("Audio5")
			elif par == 0:
				self.Pause_audio("Audio5")

		elif par.name == "Audio6":
			if par == 1:
				self.Play_audio("Audio6")
			elif par == 0:
				self.Pause_audio("Audio6")
				