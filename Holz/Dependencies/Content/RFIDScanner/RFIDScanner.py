from TDStoreTools import StorageManager
import TDFunctions as TDF

class RFIDScanner:
	def __init__(self, ownerComp):
		self.Ids = {}   # Global dictionary, wird von apply_rfid_ids() gesetzt
		return

	# called from op("datexec_create_ditionary_of_table") when Table DAT is updated
	def Sort_rfids(self, table):
		self.Ids.clear()
	
		# Zeilen durchgehen
		for row in table.rows():
			number = int(row[0].val)   # z.B. 0
			rfid   = row[1].val        # z.B. "1111111a"
					
			if number not in self.Ids:
				self.Ids[number] = []
			self.Ids[number].append(rfid)
		
		print(f"RFID Mapping erstellt: {self.Ids}")


	# called from op. MQTTin, wenn per MQTT eine neue RFID-ID empfangen wird
	def Lookup_rfid(self, rfid_code):
		print(self.Ids)

		for number, rfid_list in self.Ids.items():
			if rfid_code in rfid_list:
				print(f"Scene to play: {number}")
				op('sceneChanger').SceneChange(number, fadeTime=1)
				op("timeout_timer").par.start.pulse()


	def OnParamChanged(self, par):
		if par.name == "Audio1de":
			if par == 1:
				return

				