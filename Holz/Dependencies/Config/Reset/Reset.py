# Reset routine
# called from op.MQTT_in.op('trigger_by_incoming_mqtt')
# when mqtt msg arrives t "[topic_prefix]reset" p 1
# or t "[topic_prefix]reset/timeout" p 1


from TDStoreTools import StorageManager
import TDFunctions as TDF

class Reset:
	def __init__(self, ownerComp):
		return

	def ResetRoutine(self):
		op.Logger.Info("[Reset]: Reset")
		
		# if you wish an idle theme enable the next two lines
		# op.Language.par.Selectedlanguage = "de"

		op.ScenePlayer.par.Pause = 0

		if op.ScenePlayer.par.Latestscenemqttpayload != "democontent1":
			op.ScenePlayer.PlayScene("democontent1")
				
		else:
			op.ScenePlayer.par.Restart.pulse()
		

	def OnParamChanged(self, par):
		if par.name == "Reset":
			self.ResetRoutine()
			return
