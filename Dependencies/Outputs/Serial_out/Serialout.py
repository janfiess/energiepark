from TDStoreTools import StorageManager
import TDFunctions as TDF

class Serialout:

	def __init__(self, ownerComp):
		return
	
	def OnParamChanged(self, par):
		if par.name == "Ledtoggle":
			parent().Toggle_LED(par)
			
	def Toggle_LED(self,par):
		par = int(par)
		op.Logger.Info(f"[Serial_out]: {par}")
		op.Serial_in.op('serialdevice').send(str(par),  terminator='\n')
			
			
	

			
	
