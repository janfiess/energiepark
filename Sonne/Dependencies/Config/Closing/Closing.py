from TDStoreTools import StorageManager
import TDFunctions as TDF

import os

class Closing:
	def __init__(self, ownerComp):
		# The component to which this extension is attached
		self.ownerComp = ownerComp

		# properties
		TDF.createProperty(self, 'MyProperty', value=0, dependable=True,
						   readOnly=False)


		# stored items (persistent across saves and re-initialization):
		storedItems = [
			# Only 'name' is required...
			{'name': 'StoredProperty', 'default': None, 'readOnly': False,
			 						'property': True, 'dependable': True},
		]
	
	def SaveAllTox(self):
		# Clear all project paths
	    project.paths.clear()
	    try:
	        op.local.unstore('metadata')
	    except Exception as e:
	        # do nothing
	        pass
	
	    # Check all tox if they are external and save only if changed
	    
	    for ext_op in parent.Project.findChildren(parExpr="*'.tox'"):
	        if ext_op.dirty:
	            a = ui.messageBox('Saving modified Tox', ext_op.name, buttons=['Cancel','Ok'])
	            if bool(a): # if pressed ok
	                ext_op.save(ext_op.par.externaltox.eval())
	                debug("Saving tox:", ext_op.name)
		
	def ShutDownPC(self):
		op.Logger.Info("force shutdown PC in 10s")
		os.system('shutdown -s -t 10')
		
	def Stop_ShutDownPC(self):
		op.Logger.Info("stop shutdown")
		os.system('shutdown -a')
