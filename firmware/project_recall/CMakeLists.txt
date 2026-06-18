cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(project_recall)

target_sources(app PRIVATE
	src/main.c
	src/app_state.c
	src/audio/audio_io.c
	src/audio/decimator.c
	src/audio/codec_adpcm.c
	src/audio/chime_data.c
	src/ble/ble_conn.c
	src/ble/ble_audio_svc.c
	src/ui/led_status.c
	src/ui/epd_ui.c
	src/power/battery.c
	src/power/charger.c
	src/power/pm.c
	src/util/ringbuf.c
	src/util/task_wdt_mgr.c
)

zephyr_include_directories(src)
