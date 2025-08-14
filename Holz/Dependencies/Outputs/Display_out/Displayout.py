from TDStoreTools import StorageManager
import TDFunctions as TDF

class Displayout:
	def __init__(self, ownerComp):
		return
		
	def OnParamChanged(self, par):

		if par.name == "Reactivatedisplay":
			self.Reactivate_display()
			op.Logger.Info(f"[Display_out]: Reactivate display")
			
		if par.name == "Showui":
			self.Show_UI()
			op.Logger.Info(f"[Display_out]: Show UI (press key 8 and 9 together)")

		# if par.name == "Nextbrightness":
		# 	op("timer_brightness").par.start.pulse()

		# called by op("chopexec_sendslidervalafterchanged") after finished moving slider by hand
		if par.name == "Brightness":
			par = round(float(par),2)
			topic = op.MQTT_in.par.Topicprefix  + "display/brightness"
			if op.MQTT_in.par.Latestmqtttopic != topic  or (absTime.seconds - op.MQTT_in.par.Latestmqtttime) > 1: # nicht mehrmals hintereinander mqtt senden und empfangen
				op.MQTT_out.SendMQTT_retained(topic, par)
				if parent().par.Nextbrightness != parent().par.Brightness:
					parent().par.Nextbrightness = par
				op.Logger.Info(f"[Display_out]: op.Display_out.par.Brightness: {par}")
			
		
	def Activate_display(self):
		# selected by op("chopexec_reactivate_display")
		
		# show projection in performance window
		op.Display_out.op('window_out1').par.performance.pulse()
		# show info screen as separate window

		op.Logger.Info("[Display_out]: Activate display")
		op("projtimer").par.initialize.pulse()

	def Reactivate_display(self):
		op.Display_out.op('window_out1').par.winclose.pulse()
		op.Display_out.op("projtimer").par.start.pulse()
		
	def Show_UI(self):
		# show projection in a separate window. This opens the network, because perform window closes
		op.Display_out.op('window_out1').par.winopen.pulse()
	
		# show ui as separate window
		op.UI.op('controlwindow').par.winopen.pulse()	

	# def ChangeBrightnessAfterDelay(self):
	# 	print("ChangeBrightnessAfterDelay")
