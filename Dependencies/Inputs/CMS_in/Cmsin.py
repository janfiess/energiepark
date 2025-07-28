from TDStoreTools import StorageManager
import TDFunctions as TDF

class Cmsin:

	def __init__(self, ownerComp):
		return

	def OnParamChanged(self, par):
		if par.name == "Fakecontentjson":
			op.MQTT_out.SendMQTT_retained( op.MQTT_in.par.Topicprefix + "cms/use_fake_contentjson", int(par))
			op.Logger.Info(f"[CMS_in]: Use fake content.json: {int(par)}")
		return


