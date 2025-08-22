from TDStoreTools import StorageManager
import TDFunctions as TDF

class Startup:
	def __init__(self, ownerComp):
		return
		
	def Step1(self):

		# activate mqtts
		op.MQTT_in.par.Active = 1
		
	def Step2(self):
		# op.Logger.Info("[Startup]: startup part 2")
		# reset
		# op.Reset.ResetRoutine()
				
		# start previously played video automatcally
		# latest_mqtt_payload = op.ScenePlayer.par.Latestscenemqttpayload
		# op.Logger.Info(f"[Startup]: latest_mqtt_payload: {latest_mqtt_payload}")
		#op.ScenePlayer.PlayScene(latest_mqtt_payload)
		op.RFIDScanner.Sort_rfids( op.RFIDScanner.op("null_rfid_matching") )
		

		ui.openTextport()

	
	def Step3(self):
		# op.Logger.Info("[Startup]: startup part 3")
		# disable black display and mute audio if enabled for any reason
		# op.Display_out.par.Displayblack = 0 
		op.Audiostationen.Pause_audio("Audio1_DE")
		op.Audiostationen.Pause_audio("Audio1_EN")
		op.Audiostationen.Pause_audio("Audio2_DE")
		op.Audiostationen.Pause_audio("Audio2_EN")
		op.Audiostationen.Pause_audio("Audio3")
		op.Audiostationen.Pause_audio("Audio4")
		return
		
	def OnParamChanged(self, par):
		#if par.name == "Timelineactive":
			#me.time.play = int(par)
		return