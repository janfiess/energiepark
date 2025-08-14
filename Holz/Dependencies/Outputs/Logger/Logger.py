from TDStoreTools import StorageManager
TDF = op.TDModules.mod.TDFunctions

import logging
import logging.handlers
import sys

class Logger:
	"""
	An object that wraps basic methods from the python module.
	These methods are Debug, Info, Error, Warning, and Critical.
	For each method, the message is passed to two "handlers".
	One handler is for the textport. It determines whether the message
	should be shown in the textport. The other handler is for file
	output. It determines whether the message should be appended to a
	txt file.

	"High performance" mode would be setting both handler threshold levels
	to critical. Your code could call Debug and Info a lot, but since those
	don't meet the threshold of Critical, they would be efficiently ignored.

	A more reasonable approach is to have the textport level set to Debug
	and the file level set to Info. When you're done with the project and
	no longer debugging, then set textport to NOTSET, and leave the file level
	on Info or higher.

	"""

	def __init__(self, ownerComp):
		self.ownerComp = ownerComp
		name = str(parent().par.Loggingname)
		self.logger = logging.getLogger(name)


	"""
	Feel free to customize the five methods below to your preference.
	For example, you could make each append a message to a FIFO DAT
	or send a UDP message.
	"""

	def Debug(self, msg):
		self.logger.debug(msg)


	def Info(self, msg):
		self.logger.info(msg)
    
    
	def Error(self, msg):
		self.logger.error(msg)
    
    
	def Warning(self, msg):
		self.logger.warning(msg)


	def Critical(self, msg):
		self.logger.critical(msg)

	def ReinitializeLogger(self):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		logging_name = str(self.ownerComp.par.Loggingname)
					
		# performance optimizations
		logging.logThreads = 0
		logging.logProcesses = 0
		
		rootlogger = logging.getLogger(logging_name)
		
		# remove previous handlers
		rootlogger.handlers = []
		
		self.UpdateRootLevel()
		
		self.UpdateFileHandlerLevel()
		self.UpdateTextportHandlerLevel()

	def UpdateFileHandlerLevel(self, log_format='%(asctime)s - %(levelname)s - %(message)s'):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		logging_name = str(self.ownerComp.par.Loggingname)
		rootlogger = logging.getLogger(logging_name)

		level = self.lookup_level(str(self.ownerComp.par.Filelevel))

		# Look for an existing file handler to update and return early.			
		for h in rootlogger.handlers:
			if type(h) is logging.handlers.TimedRotatingFileHandler:
				h.setLevel(level)
				msg = "Changed file logger '{0}' level to '{1}'".format(logging_name, level)
				rootlogger.info(msg)
				# return early because we updated an existing file handler.
				return

		# We didn't find a file handler, so create it.
		file_when = self.lookup_file_when(str(self.ownerComp.par.Filewhen))
		file_interval = float(self.ownerComp.par.Fileinterval) # A number
		file_backup_count = int(self.ownerComp.par.Filebackupcount) # Number of text files to keep. 0 means no limit
		logging_file = str(self.ownerComp.par.Loggingfile)

		# create file handler
		fh = logging.handlers.TimedRotatingFileHandler(logging_file, when=file_when,
			interval=file_interval, backupCount=file_backup_count, encoding=None,
			delay=False, utc=False)
		fh.setLevel(level)
		formatter = logging.Formatter(log_format, datefmt='%Y-%m-%d %H:%M:%S')
		fh.setFormatter(formatter)
		
		rootlogger.addHandler(fh)


	def UpdateTextportHandlerLevel(self, log_format='%(asctime)s - %(levelname)s - %(message)s'):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		logging_name = str(self.ownerComp.par.Loggingname)
		rootlogger = logging.getLogger(logging_name)

		level = self.lookup_level(str(self.ownerComp.par.Textportlevel))

		# Look for an existing textport handler to update and return early.
		for h in rootlogger.handlers:
			if type(h) is logging.StreamHandler:
				h.setLevel(level)
				msg = "Changed textport logger '{0}' level to '{1}'".format(logging_name, level)
				rootlogger.info(msg)
				# return early because we updated an existing textport handler.
				return

		# We didn't find a textport handler, so create it.
		ch = logging.StreamHandler(sys.stdout)
		ch.setLevel(level)
		formatter = logging.Formatter(log_format, datefmt='%Y-%m-%d %H:%M:%S')
		ch.setFormatter(formatter)

		rootlogger.addHandler(ch)


	def RemoveFileHandler(self):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		logging_name = str(self.ownerComp.par.Loggingname)
		rootlogger = logging.getLogger(logging_name)
		rootlogger.handlers = [h for h in rootlogger.handlers if not isinstance(h, logging.TimedRotatingFileHandler)]
		
		
	def UpdateRootLevel(self):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		logging_name = str(self.ownerComp.par.Loggingname)
		rootlogger = logging.getLogger(logging_name)
		
		textport_level = self.lookup_level(str(self.ownerComp.par.Textportlevel))
		file_level = self.lookup_level(str(self.ownerComp.par.Filelevel))
				
		root_level = self.getRootLevel(textport_level, file_level)

		rootlogger.setLevel(root_level)
		
		
	def lookup_level(self, level):
		"""
		Although this method is public, there's no reason for the user to call it.
		"""
		levelMap = {
			'Notset': 'NOTSET',
			'Debug': 'DEBUG',
			'Info': 'INFO',
			'Warning': 'WARNING',
			'Error': 'ERROR',
			'Critical': 'CRITICAL'
		}
		
		if level in levelMap:
			return levelMap[level]
		else:
			msg = 'Logging level {0} was not found.'.format(level)
			debug(msg)
			self.ownerComp.Critical(msg)

			return 'NOTSET'
			
			
	def lookup_file_when(self, file_when):

		whenMap = {
			'Seconds': 'S',
			'Minutes': 'M',
			'Hours': 'H',
			'Days': 'D',
			'Midnight': 'midnight',
			'Monday': 'W0',
			'Tuesday': 'W1',
			'Wednesday': 'W2',
			'Thursday': 'W3',
			'Friday': 'W4',
			'Saturday': 'W5',
			'Sunday': 'W6',
		}
	
		if file_when in whenMap:
			return whenMap[file_when]
		else:
			msg = 'Filewhen {0} was not found.'.format(file_when)
			debug(msg)
			self.ownerComp.Critical(msg)
			
			return 'midnight'
		        
	   
	def getRootLevel(self, textport_level, file_level):
		textport_lev = logging.getLevelName(textport_level)
		file_lev = logging.getLevelName(file_level)

		if textport_lev == 0 and file_lev == 0:
			# then they're both NOTSET, so for max performance return
			# a level above CRITICAL.
			return 60

		if textport_lev == 0:
			return file_lev
		elif file_lev == 0:
			return textport_lev

		return min(textport_lev, file_lev)
		
	def OnParamChanged(self, par):
		#if par.name == "Reactivatedisplay":
			#self.Reactivate_display()
		return
