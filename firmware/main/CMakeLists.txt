set(srcs "./app.cpp"
	"./ledclocksensor.cpp"
  "./dht22task.cpp"
  "./mhz19btask.cpp"
  "./menus/calibration_menu.cpp"
  "./menus/menu_state.cpp"
  "./menus/appbase_menu.cpp"
  "./menus/gui_list_processor.cpp"
  "./menus/wifi_menu.cpp"
  "./menus/virtual_key_board.cpp"
  "./menus/setting_menu.cpp"
  "./menus/menu3d.cpp"
  "./menus/3d/renderer.cpp"
)

idf_component_register(SRCS "${srcs}"
		INCLUDE_DIRS ../components/esp-etl/etl/include/ ../components/libesp
		)
