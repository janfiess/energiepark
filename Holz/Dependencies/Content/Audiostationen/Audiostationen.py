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
				