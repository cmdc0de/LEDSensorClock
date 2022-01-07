set(srcs "./app.cpp"
	"./ledclocksensor.cpp"
  "./menu/calibration_menu.cpp"
  "./menu/menu_state.cpp"
  "./menu/appbase_menu.cpp"
  "./menu/gui_list_processor.cpp"
)

idf_component_register(SRCS "${srcs}"
		INCLUDE_DIRS ../components/esp-etl/etl/include/ ../components/libesp
		)
