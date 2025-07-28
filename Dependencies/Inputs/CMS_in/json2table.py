import json 

def cook(scriptOp):

	#get the raw json string
	jsonRaw = scriptOp.inputs[0].text

	#convert the json string to a python dict	
	fullDict = json.loads(jsonRaw)

	scriptOp.clear()
	scriptOp.appendRow( ["mqtt_payload", "loop", "successor", "is_generative_content", "button_mqtt_action", "button_pos_x", "button_pos_y", "video_en", "extra_en", "generative_content_en", "button_text_en", "video_de", "extra_de", "generative_content_de", "button_text_en", "video_fr", "extra_fr", "generative_content_fr", "button_text_fr"] )
	
	for dataset in fullDict:
		mqtt_payload = dataset["mqtt_message"] if dataset["mqtt_message"] is not None else ""
		loop = dataset["Looped"] if dataset["Looped"] is not None else ""
		successor = dataset["Successor"] if dataset["Successor"] is not None else ""
		
		# is generative content
			
		try:
			is_generative_content = dataset["is_generative_content"]
		except AttributeError:
			is_generative_content = 0



		# button_mqtt_action

		button_mqtt_action = dataset["button_mqtt_action"] if dataset["button_mqtt_action"] is not None else ""
		button_pos_x = dataset["button_pos_x"] if dataset["button_pos_x"] is not None else "" 
		button_pos_y= dataset["button_pos_y"] if dataset["button_pos_y"] is not None else ""



		##### en
		
		if not dataset["localizations"].get("en"):               # wenn nicht einmal "en" angelegt ist, lasse das Feld leer
			video_en = ""
			extra_en = ""
			generative_content_en = ""
			button_text_en = "huhu"

		# - en video

		if dataset["localizations"]["en"].get("Video") and dataset["localizations"].get("en"):    # wenn in "url" ein video eingeordnet ist, nehme dieses
			try:
				video_en = dataset["localizations"]["en"]["Video"]["url"]      # wenn zwar "en" angelegt ist, aber danach kein Eintrag kommt, lasse das Feld leer
			except AttributeError:
				video_en = ""
		else:
			video_en = ""

		# - en extra

		if dataset["localizations"]["en"].get("Extra") and dataset["localizations"].get("en"):
			try:
				extra_en = dataset["localizations"]["en"]["Extra"]["url"]
			except AttributeError:
				extra_en = ""
		else: 
			extra_en = ""

		# - en generative content
			
		if dataset["localizations"]["en"].get("Generative_content") and dataset["localizations"].get("en"):
			try:
				generative_content_en = dataset["localizations"]["en"]["Generative_content"]
			except AttributeError:
				generative_content_en = 0
		else: 
			generative_content_en = 0

		# - en button text
			
		if dataset["localizations"]["en"].get("button_text") and dataset["localizations"].get("en"):
			try:
				button_text_en = dataset["localizations"]["en"]["button_text"]
			except AttributeError:
				button_text_en = ""
		else: 
			button_text_en = 0


		




		##### de
		# - de video

		if not dataset["localizations"].get("de-CH"):                                # wenn nicht einmal "de-ch" angelegt ist, übernehme englischen eintrag
			video_de = video_en if video_en is not None else ""
			extra_de = extra_en if extra_en is not None else ""
			generative_content_de = generative_content_en if generative_content_en is not None else ""
			button_text_de = button_text_en if button_text_en is not None else ""
		
		
		if dataset["localizations"]["de-CH"].get("Video") and dataset["localizations"].get("de-CH"):  # wenn in "url" ein video eingeordnet ist, nehme dieses
			try:
				video_de = dataset["localizations"]["de-CH"]["Video"]["url"]
			except AttributeError:
				video_de = video_en if video_en is not None else ""
		else:                                                                        # wenn zwar "de-ch" angelegt ist, aber danach kein Eintrag kommt, übernehme englischen Eintrag
			video_de = video_en if video_en is not None else ""
		
		# - de extra

		if dataset["localizations"]["de-CH"].get("Extra") and dataset["localizations"].get("de-CH"):
			try:
				extra_de = dataset["localizations"]["de-CH"]["Extra"]["url"]
			except AttributeError:
				extra_de = extra_en if extra_en is not None else ""
		else:
			extra_de = extra_en if extra_en is not None else ""



		# - de generative content
			
		if dataset["localizations"]["de-CH"].get("Generative_content") and dataset["localizations"].get("de-CH"):
			try:
				generative_content_de = dataset["localizations"]["de-CH"]["Generative_content"]
			except AttributeError:
				generative_content_de = ""
		else: 
			generative_content_de = ""


		# - de button text
			
		if dataset["localizations"]["de-CH"].get("button_text") and dataset["localizations"].get("de-CH"):
			try:
				button_text_de = dataset["localizations"]["de-CH"]["button_text"]
			except AttributeError:
				button_text_de = ""
		else: 
			button_text_de = ""
			
			
			
			
		##### fr
		# - fr video

		if not dataset["localizations"].get("fr"):                                # wenn nicht einmal "de-ch" angelegt ist, übernehme englischen eintrag
			video_fr = video_en if video_en is not None else ""
			extra_fr = extra_en if extra_en is not None else ""
			generative_content_fr = generative_content_en if generative_content_en is not None else ""
			button_text_fr = button_text_en if button_text_en is not None else ""


		if dataset["localizations"]["fr"].get("Video") and dataset["localizations"].get("fr"):  # wenn in "url" ein video eingeordnet ist, nehme dieses
			try:
				video_fr = dataset["localizations"]["fr"]["Video"]["url"]
			except AttributeError:
				video_fr = video_en if video_en is not None else ""
		else:                                                                        # wenn zwar "de-ch" angelegt ist, aber danach kein Eintrag kommt, übernehme englischen Eintrag
			video_fr = video_en if video_en is not None else ""
		
		# - fr extra

		if dataset["localizations"]["fr"].get("Extra") and dataset["localizations"].get("fr"):
			try:
				extra_fr = dataset["localizations"]["fr"]["Extra"]["url"]
			except AttributeError:
				extra_fr = extra_en if extra_en is not None else ""
		else:
			extra_fr = extra_en if extra_en is not None else ""

		# - fr generative content
			
		if dataset["localizations"]["fr"].get("Generative_content") and dataset["localizations"].get("fr"):
			try:
				generative_content_fr = dataset["localizations"]["fr"]["Generative_content"]
			except AttributeError:
				generative_content_fr = ""
		else: 
			generative_content_fr = ""


		# - fr button text
			
		if dataset["localizations"]["fr"].get("button_text") and dataset["localizations"].get("fr"):
			try:
				button_text_fr = dataset["localizations"]["fr"]["button_text"]
			except AttributeError:
				button_text_fr = ""
		else: 
			button_text_fr = ""



		scriptOp.appendRow( [mqtt_payload, loop, successor, is_generative_content, button_mqtt_action, button_pos_x, button_pos_y, video_en, extra_en, generative_content_en, button_text_en, video_de, extra_de, generative_content_de, button_text_de, video_fr, extra_fr, generative_content_fr, button_text_fr] )