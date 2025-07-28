from TDStoreTools import StorageManager
import TDFunctions as TDF

class ScenePlayer:
	def __init__(self, ownerComp):
		self.scenes = op.Footage.findChildren(name = "footage*", parName = "Mqttpayload", maxDepth=1)
		# self.nextscene = op.ScenePlayer.par.Currentreplicatorinstance
		# "accepted_payloads" is a list of expected payloads coming from the content.jsons when a mqtt msg with the topic "[mqtt_topic_prefix]play" arrives
		self.accepted_payloads_list = []
		self.Update_list_of_accepted_payloads()
		self.moviefile = op.Footage.op(f"{op.ScenePlayer.par.Currentreplicatorinstance}").op("videofile")

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
			


	def Play_new(self):
		#print("start play new")
		parent().par.Prevreplicatorinstance = parent().par.Currentreplicatorinstance
		parent().par.Currentreplicatorinstance = parent().par.Nextreplicatorinstance  # used to select the right scene
		if str(parent().par.Currentreplicatorinstance) == "":
			parent().par.Currentreplicatorinstance = op.ScenePlayer.par.Prevreplicatorinstance
		parent().par.Nextreplicatorinstance  = None

		self.moviefile = op.Footage.op(f"{op.ScenePlayer.par.Currentreplicatorinstance}").op("videofile")
	
		if (absTime.seconds - op.ScenePlayer.par.Latestscenemqtttime) > 1:
			self.moviefile.par.cuepoint = 0
			self.moviefile.par.cuepulse.pulse()

		# self.moviefile.par.play = 1
		parent().par.Pause = 0
		op.Logger.Info(f"[ScenePlayer]: playing now {parent().par.Latestscenemqttpayload}")

		
	def GetSceneName(self, payload):
		scene = None
		for sc in self.scenes:
			scene_mqtt_payload_from_cms = op(sc.path).par.Mqttpayload

			if str(scene_mqtt_payload_from_cms) in str(payload):
				scene = sc
				op.ScenePlayer.par.Sceneindex = int(op(sc.path).name[7:])
				break
		# print(f"[ScenePlayer]: GetSceneName: payload: {payload}, scene.name: {scene.name}")
		return scene.name
	

	# pause video
	def Pause(self):
		self.moviefile.par.play = 0
		parent().par.Pause = 1
		op.Timer.par.Pausepressed.pulse()


	# triggered from topic:"[topicprefix]player" payload: "continue")
	def Continue(self):
		self.moviefile.par.play = 1
		parent().par.Pause = 0
		op.Timer.par.Resetalltimers.pulse()
		op.Logger.Info(f"[ScenePlayer]: Continue {self.moviefile.name}")

	def Restart(self):
		self.moviefile.par.cuepulse.pulse()

	def OnParamChanged(self, par):
		if par.name == "Pause":
			if par == 1:
				self.Pause()
				op.Logger.Info(f"[ScenePlayer]: Pause video")
			elif par == 0:
				self.Continue()
				op.Logger.Info(f"[ScenePlayer]: Continue video now")
			
		elif par.name == "Restart":
			self.Restart()
			op.Logger.Info(f"[ScenePlayer]: restart video now")

		elif par.name == "Updateexpectedpayloads":
			self.Update_list_of_accepted_payloads()
			op.Logger.Info(f"[ScenePlayer]: updated list of accepted payloads")


		elif par.name == "Fadeintime":	
			par = round(float(par),1)
			op.Logger.Info(f"[ScenePlayer]: Changed fadein time to {par}s")
			# send status only via mqtt if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "player/fadeintime"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, par)

		elif par.name == "Fadeouttime":	
			par = round(float(par),1)		
			op.Logger.Info(f"[ScenePlayer]: Changed fadeout time to {par}s")
			# send status only via mqtt if it didn't come from the mqtt broker
			topic = op.MQTT_in.par.Topicprefix  + "player/fadeouttime"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, par)
			
	def Update_list_of_accepted_payloads(self):
		accepted_payloads_table = op.ScenePlayer.op("null_expected_videos_from_cms")
		self.accepted_payloads_list = []

		for row in range(1, accepted_payloads_table.numRows):
			self.accepted_payloads_list.append(str(accepted_payloads_table[row, 0]))

		# op.Logger.Info(f"[ScenePlayer]: Accepted_payloads_list: {self.accepted_payloads_list}")