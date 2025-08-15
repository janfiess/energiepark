from TDStoreTools import StorageManager
import TDFunctions as TDF

class Scanner:
	def __init__(self, ownerComp):
		return

	def Identify(self, rfid_id):
		print(rfid_id)

	def OnParamChanged(self, par):
		if par.name == "Audio1de":
			if par == 1:
				return

				